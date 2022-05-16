#include <vxldollar/node/confirmation_solicitor.hpp>
#include <vxldollar/node/election.hpp>
#include <vxldollar/node/network.hpp>
#include <vxldollar/node/node.hpp>

#include <boost/format.hpp>

using namespace std::chrono;

std::chrono::milliseconds vxldollar::election::base_latency () const
{
	return node.network_params.network.is_dev_network () ? 25ms : 1000ms;
}

vxldollar::election_vote_result::election_vote_result (bool replay_a, bool processed_a)
{
	replay = replay_a;
	processed = processed_a;
}

vxldollar::election::election (vxldollar::node & node_a, std::shared_ptr<vxldollar::block> const & block_a, std::function<void (std::shared_ptr<vxldollar::block> const &)> const & confirmation_action_a, std::function<void (vxldollar::account const &)> const & live_vote_action_a, vxldollar::election_behavior election_behavior_a) :
	confirmation_action (confirmation_action_a),
	live_vote_action (live_vote_action_a),
	behavior (election_behavior_a),
	node (node_a),
	status ({ block_a, 0, 0, std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::system_clock::now ().time_since_epoch ()), std::chrono::duration_values<std::chrono::milliseconds>::zero (), 0, 1, 0, vxldollar::election_status_type::ongoing }),
	height (block_a->sideband ().height),
	root (block_a->root ()),
	qualified_root (block_a->qualified_root ())
{
	last_votes.emplace (vxldollar::account::null (), vxldollar::vote_info{ std::chrono::steady_clock::now (), 0, block_a->hash () });
	last_blocks.emplace (block_a->hash (), block_a);
	if (node.config.enable_voting && node.wallets.reps ().voting > 0)
	{
		node.active.generator.add (root, block_a->hash ());
	}
}

void vxldollar::election::confirm_once (vxldollar::unique_lock<vxldollar::mutex> & lock_a, vxldollar::election_status_type type_a)
{
	debug_assert (lock_a.owns_lock ());
	// This must be kept above the setting of election state, as dependent confirmed elections require up to date changes to election_winner_details
	vxldollar::unique_lock<vxldollar::mutex> election_winners_lk (node.active.election_winner_details_mutex);
	if (state_m.exchange (vxldollar::election::state_t::confirmed) != vxldollar::election::state_t::confirmed && (node.active.election_winner_details.count (status.winner->hash ()) == 0))
	{
		node.active.election_winner_details.emplace (status.winner->hash (), shared_from_this ());
		election_winners_lk.unlock ();
		status.election_end = std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::system_clock::now ().time_since_epoch ());
		status.election_duration = std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::steady_clock::now () - election_start);
		status.confirmation_request_count = confirmation_request_count;
		status.block_count = vxldollar::narrow_cast<decltype (status.block_count)> (last_blocks.size ());
		status.voter_count = vxldollar::narrow_cast<decltype (status.voter_count)> (last_votes.size ());
		status.type = type_a;
		auto const status_l = status;
		lock_a.unlock ();
		node.process_confirmed (status_l);
		node.background ([node_l = node.shared (), status_l, confirmation_action_l = confirmation_action] () {
			if (confirmation_action_l)
			{
				confirmation_action_l (status_l.winner);
			}
		});
	}
	else
	{
		lock_a.unlock ();
	}
}

bool vxldollar::election::valid_change (vxldollar::election::state_t expected_a, vxldollar::election::state_t desired_a) const
{
	bool result = false;
	switch (expected_a)
	{
		case vxldollar::election::state_t::passive:
			switch (desired_a)
			{
				case vxldollar::election::state_t::active:
				case vxldollar::election::state_t::confirmed:
				case vxldollar::election::state_t::expired_unconfirmed:
					result = true;
					break;
				default:
					break;
			}
			break;
		case vxldollar::election::state_t::active:
			switch (desired_a)
			{
				case vxldollar::election::state_t::confirmed:
				case vxldollar::election::state_t::expired_unconfirmed:
					result = true;
					break;
				default:
					break;
			}
			break;
		case vxldollar::election::state_t::confirmed:
			switch (desired_a)
			{
				case vxldollar::election::state_t::expired_confirmed:
					result = true;
					break;
				default:
					break;
			}
			break;
		case vxldollar::election::state_t::expired_unconfirmed:
		case vxldollar::election::state_t::expired_confirmed:
			break;
	}
	return result;
}

bool vxldollar::election::state_change (vxldollar::election::state_t expected_a, vxldollar::election::state_t desired_a)
{
	bool result = true;
	if (valid_change (expected_a, desired_a))
	{
		if (state_m.compare_exchange_strong (expected_a, desired_a))
		{
			state_start = std::chrono::steady_clock::now ().time_since_epoch ();
			result = false;
		}
	}
	return result;
}

void vxldollar::election::send_confirm_req (vxldollar::confirmation_solicitor & solicitor_a)
{
	if ((base_latency () * (optimistic () ? 10 : 5)) < (std::chrono::steady_clock::now () - last_req))
	{
		vxldollar::lock_guard<vxldollar::mutex> guard (mutex);
		if (!solicitor_a.add (*this))
		{
			last_req = std::chrono::steady_clock::now ();
			++confirmation_request_count;
		}
	}
}

void vxldollar::election::transition_active ()
{
	state_change (vxldollar::election::state_t::passive, vxldollar::election::state_t::active);
}

bool vxldollar::election::confirmed () const
{
	return state_m == vxldollar::election::state_t::confirmed || state_m == vxldollar::election::state_t::expired_confirmed;
}

bool vxldollar::election::failed () const
{
	return state_m == vxldollar::election::state_t::expired_unconfirmed;
}

void vxldollar::election::broadcast_block (vxldollar::confirmation_solicitor & solicitor_a)
{
	if (base_latency () * 15 < std::chrono::steady_clock::now () - last_block)
	{
		vxldollar::lock_guard<vxldollar::mutex> guard (mutex);
		if (!solicitor_a.broadcast (*this))
		{
			last_block = std::chrono::steady_clock::now ();
		}
	}
}

bool vxldollar::election::transition_time (vxldollar::confirmation_solicitor & solicitor_a)
{
	bool result = false;
	switch (state_m)
	{
		case vxldollar::election::state_t::passive:
			if (base_latency () * passive_duration_factor < std::chrono::steady_clock::now ().time_since_epoch () - state_start.load ())
			{
				state_change (vxldollar::election::state_t::passive, vxldollar::election::state_t::active);
			}
			break;
		case vxldollar::election::state_t::active:
			broadcast_block (solicitor_a);
			send_confirm_req (solicitor_a);
			break;
		case vxldollar::election::state_t::confirmed:
			if (base_latency () * confirmed_duration_factor < std::chrono::steady_clock::now ().time_since_epoch () - state_start.load ())
			{
				result = true;
				state_change (vxldollar::election::state_t::confirmed, vxldollar::election::state_t::expired_confirmed);
			}
			break;
		case vxldollar::election::state_t::expired_unconfirmed:
		case vxldollar::election::state_t::expired_confirmed:
			debug_assert (false);
			break;
	}
	auto const optimistic_expiration_time = 60 * 1000;
	auto const expire_time = std::chrono::milliseconds (optimistic () ? optimistic_expiration_time : 5 * 60 * 1000);
	if (!confirmed () && expire_time < std::chrono::steady_clock::now () - election_start)
	{
		vxldollar::lock_guard<vxldollar::mutex> guard (mutex);
		// It is possible the election confirmed while acquiring the mutex
		// state_change returning true would indicate it
		if (!state_change (state_m.load (), vxldollar::election::state_t::expired_unconfirmed))
		{
			result = true;
			if (node.config.logging.election_expiration_tally_logging ())
			{
				log_votes (tally_impl (), "Election expired: ");
			}
			status.type = vxldollar::election_status_type::stopped;
		}
	}
	return result;
}

bool vxldollar::election::have_quorum (vxldollar::tally_t const & tally_a) const
{
	auto i (tally_a.begin ());
	++i;
	auto second (i != tally_a.end () ? i->first : 0);
	auto delta_l (node.online_reps.delta ());
	release_assert (tally_a.begin ()->first >= second);
	bool result{ (tally_a.begin ()->first - second) >= delta_l };
	return result;
}

vxldollar::tally_t vxldollar::election::tally () const
{
	vxldollar::lock_guard<vxldollar::mutex> guard (mutex);
	return tally_impl ();
}

vxldollar::tally_t vxldollar::election::tally_impl () const
{
	std::unordered_map<vxldollar::block_hash, vxldollar::uint128_t> block_weights;
	std::unordered_map<vxldollar::block_hash, vxldollar::uint128_t> final_weights_l;
	for (auto const & [account, info] : last_votes)
	{
		auto rep_weight (node.ledger.weight (account));
		block_weights[info.hash] += rep_weight;
		if (info.timestamp == std::numeric_limits<uint64_t>::max ())
		{
			final_weights_l[info.hash] += rep_weight;
		}
	}
	last_tally = block_weights;
	vxldollar::tally_t result;
	for (auto const & [hash, amount] : block_weights)
	{
		auto block (last_blocks.find (hash));
		if (block != last_blocks.end ())
		{
			result.emplace (amount, block->second);
		}
	}
	// Calculate final votes sum for winner
	if (!final_weights_l.empty () && !result.empty ())
	{
		auto winner_hash (result.begin ()->second->hash ());
		auto find_final (final_weights_l.find (winner_hash));
		if (find_final != final_weights_l.end ())
		{
			final_weight = find_final->second;
		}
	}
	return result;
}

void vxldollar::election::confirm_if_quorum (vxldollar::unique_lock<vxldollar::mutex> & lock_a)
{
	debug_assert (lock_a.owns_lock ());
	auto tally_l (tally_impl ());
	debug_assert (!tally_l.empty ());
	auto winner (tally_l.begin ());
	auto block_l (winner->second);
	auto const & winner_hash_l (block_l->hash ());
	status.tally = winner->first;
	status.final_tally = final_weight;
	auto const & status_winner_hash_l (status.winner->hash ());
	vxldollar::uint128_t sum (0);
	for (auto & i : tally_l)
	{
		sum += i.first;
	}
	if (sum >= node.online_reps.delta () && winner_hash_l != status_winner_hash_l)
	{
		status.winner = block_l;
		remove_votes (status_winner_hash_l);
		node.block_processor.force (block_l);
	}
	if (have_quorum (tally_l))
	{
		if (node.ledger.cache.final_votes_confirmation_canary.load () && !is_quorum.exchange (true) && node.config.enable_voting && node.wallets.reps ().voting > 0)
		{
			auto hash = status.winner->hash ();
			lock_a.unlock ();
			node.active.final_generator.add (root, hash);
			lock_a.lock ();
		}
		if (!node.ledger.cache.final_votes_confirmation_canary.load () || final_weight >= node.online_reps.delta ())
		{
			if (node.config.logging.vote_logging () || (node.config.logging.election_fork_tally_logging () && last_blocks.size () > 1))
			{
				log_votes (tally_l);
			}
			confirm_once (lock_a, vxldollar::election_status_type::active_confirmed_quorum);
		}
	}
}

void vxldollar::election::log_votes (vxldollar::tally_t const & tally_a, std::string const & prefix_a) const
{
	std::stringstream tally;
	std::string line_end (node.config.logging.single_line_record () ? "\t" : "\n");
	tally << boost::str (boost::format ("%1%%2%Vote tally for root %3%, final weight:%4%") % prefix_a % line_end % root.to_string () % final_weight);
	for (auto i (tally_a.begin ()), n (tally_a.end ()); i != n; ++i)
	{
		tally << boost::str (boost::format ("%1%Block %2% weight %3%") % line_end % i->second->hash ().to_string () % i->first.convert_to<std::string> ());
	}
	for (auto i (last_votes.begin ()), n (last_votes.end ()); i != n; ++i)
	{
		if (i->first != nullptr)
		{
			tally << boost::str (boost::format ("%1%%2% %3% %4%") % line_end % i->first.to_account () % std::to_string (i->second.timestamp) % i->second.hash.to_string ());
		}
	}
	node.logger.try_log (tally.str ());
}

std::shared_ptr<vxldollar::block> vxldollar::election::find (vxldollar::block_hash const & hash_a) const
{
	std::shared_ptr<vxldollar::block> result;
	vxldollar::lock_guard<vxldollar::mutex> guard (mutex);
	if (auto existing = last_blocks.find (hash_a); existing != last_blocks.end ())
	{
		result = existing->second;
	}
	return result;
}

vxldollar::election_vote_result vxldollar::election::vote (vxldollar::account const & rep, uint64_t timestamp_a, vxldollar::block_hash const & block_hash_a)
{
	auto replay (false);
	auto online_stake (node.online_reps.trended ());
	auto weight (node.ledger.weight (rep));
	auto should_process (false);
	if (node.network_params.network.is_dev_network () || weight > node.minimum_principal_weight (online_stake))
	{
		unsigned int cooldown;
		if (weight < online_stake / 100) // 0.1% to 1%
		{
			cooldown = 15;
		}
		else if (weight < online_stake / 20) // 1% to 5%
		{
			cooldown = 5;
		}
		else // 5% or above
		{
			cooldown = 1;
		}

		vxldollar::unique_lock<vxldollar::mutex> lock (mutex);

		auto last_vote_it (last_votes.find (rep));
		if (last_vote_it == last_votes.end ())
		{
			should_process = true;
		}
		else
		{
			auto last_vote_l (last_vote_it->second);
			if (last_vote_l.timestamp < timestamp_a || (last_vote_l.timestamp == timestamp_a && last_vote_l.hash < block_hash_a))
			{
				auto max_vote = timestamp_a == std::numeric_limits<uint64_t>::max () && last_vote_l.timestamp < timestamp_a;
				auto past_cooldown = last_vote_l.time <= std::chrono::steady_clock::now () - std::chrono::seconds (cooldown);
				should_process = max_vote || past_cooldown;
			}
			else
			{
				replay = true;
			}
		}
		if (should_process)
		{
			node.stats.inc (vxldollar::stat::type::election, vxldollar::stat::detail::vote_new);
			last_votes[rep] = { std::chrono::steady_clock::now (), timestamp_a, block_hash_a };
			live_vote_action (rep);
			if (!confirmed ())
			{
				confirm_if_quorum (lock);
			}
		}
	}
	return vxldollar::election_vote_result (replay, should_process);
}

bool vxldollar::election::publish (std::shared_ptr<vxldollar::block> const & block_a)
{
	vxldollar::unique_lock<vxldollar::mutex> lock (mutex);

	// Do not insert new blocks if already confirmed
	auto result (confirmed ());
	if (!result && last_blocks.size () >= max_blocks && last_blocks.find (block_a->hash ()) == last_blocks.end ())
	{
		if (!replace_by_weight (lock, block_a->hash ()))
		{
			result = true;
			node.network.publish_filter.clear (block_a);
		}
		debug_assert (lock.owns_lock ());
	}
	if (!result)
	{
		auto existing = last_blocks.find (block_a->hash ());
		if (existing == last_blocks.end ())
		{
			last_blocks.emplace (std::make_pair (block_a->hash (), block_a));
		}
		else
		{
			result = true;
			existing->second = block_a;
			if (status.winner->hash () == block_a->hash ())
			{
				status.winner = block_a;
				node.network.flood_block (block_a, vxldollar::buffer_drop_policy::no_limiter_drop);
			}
		}
	}
	/*
	Result is true if:
	1) election is confirmed or expired
	2) given election contains 10 blocks & new block didn't receive enough votes to replace existing blocks
	3) given block in already in election & election contains less than 10 blocks (replacing block content with new)
	*/
	return result;
}

std::size_t vxldollar::election::insert_inactive_votes_cache (vxldollar::inactive_cache_information const & cache_a)
{
	vxldollar::unique_lock<vxldollar::mutex> lock (mutex);
	for (auto const & [rep, timestamp] : cache_a.voters)
	{
		auto inserted (last_votes.emplace (rep, vxldollar::vote_info{ std::chrono::steady_clock::time_point::min (), timestamp, cache_a.hash }));
		if (inserted.second)
		{
			node.stats.inc (vxldollar::stat::type::election, vxldollar::stat::detail::vote_cached);
		}
	}
	if (!confirmed ())
	{
		if (!cache_a.voters.empty ())
		{
			auto delay (std::chrono::duration_cast<std::chrono::seconds> (std::chrono::steady_clock::now () - cache_a.arrival));
			if (delay > late_blocks_delay)
			{
				node.stats.inc (vxldollar::stat::type::election, vxldollar::stat::detail::late_block);
				node.stats.add (vxldollar::stat::type::election, vxldollar::stat::detail::late_block_seconds, vxldollar::stat::dir::in, delay.count (), true);
			}
		}
		if (last_votes.size () > 1) // null account
		{
			// Even if no votes were in cache, they could be in the election
			confirm_if_quorum (lock);
		}
	}
	return cache_a.voters.size ();
}

bool vxldollar::election::optimistic () const
{
	return behavior == vxldollar::election_behavior::optimistic;
}

vxldollar::election_extended_status vxldollar::election::current_status () const
{
	vxldollar::lock_guard<vxldollar::mutex> guard (mutex);
	vxldollar::election_status status_l = status;
	status_l.confirmation_request_count = confirmation_request_count;
	status_l.block_count = vxldollar::narrow_cast<decltype (status_l.block_count)> (last_blocks.size ());
	status_l.voter_count = vxldollar::narrow_cast<decltype (status_l.voter_count)> (last_votes.size ());
	return vxldollar::election_extended_status{ status_l, last_votes, tally_impl () };
}

std::shared_ptr<vxldollar::block> vxldollar::election::winner () const
{
	vxldollar::lock_guard<vxldollar::mutex> guard (mutex);
	return status.winner;
}

void vxldollar::election::generate_votes () const
{
	if (node.config.enable_voting && node.wallets.reps ().voting > 0)
	{
		vxldollar::unique_lock<vxldollar::mutex> lock (mutex);
		if (confirmed () || have_quorum (tally_impl ()))
		{
			auto hash = status.winner->hash ();
			lock.unlock ();
			node.active.final_generator.add (root, hash);
			lock.lock ();
		}
		else
		{
			node.active.generator.add (root, status.winner->hash ());
		}
	}
}

void vxldollar::election::remove_votes (vxldollar::block_hash const & hash_a)
{
	debug_assert (!mutex.try_lock ());
	if (node.config.enable_voting && node.wallets.reps ().voting > 0)
	{
		// Remove votes from election
		auto list_generated_votes (node.history.votes (root, hash_a));
		for (auto const & vote : list_generated_votes)
		{
			last_votes.erase (vote->account);
		}
		// Clear votes cache
		node.history.erase (root);
	}
}

void vxldollar::election::remove_block (vxldollar::block_hash const & hash_a)
{
	debug_assert (!mutex.try_lock ());
	if (status.winner->hash () != hash_a)
	{
		if (auto existing = last_blocks.find (hash_a); existing != last_blocks.end ())
		{
			for (auto i (last_votes.begin ()); i != last_votes.end ();)
			{
				if (i->second.hash == hash_a)
				{
					i = last_votes.erase (i);
				}
				else
				{
					++i;
				}
			}
			node.network.publish_filter.clear (existing->second);
			last_blocks.erase (hash_a);
		}
	}
}

bool vxldollar::election::replace_by_weight (vxldollar::unique_lock<vxldollar::mutex> & lock_a, vxldollar::block_hash const & hash_a)
{
	debug_assert (lock_a.owns_lock ());
	vxldollar::block_hash replaced_block (0);
	auto winner_hash (status.winner->hash ());
	// Sort existing blocks tally
	std::vector<std::pair<vxldollar::block_hash, vxldollar::uint128_t>> sorted;
	sorted.reserve (last_tally.size ());
	std::copy (last_tally.begin (), last_tally.end (), std::back_inserter (sorted));
	lock_a.unlock ();
	// Sort in ascending order
	std::sort (sorted.begin (), sorted.end (), [] (auto const & left, auto const & right) { return left.second < right.second; });
	// Replace if lowest tally is below inactive cache new block weight
	auto inactive_existing (node.active.find_inactive_votes_cache (hash_a));
	auto inactive_tally (inactive_existing.status.tally);
	if (inactive_tally > 0 && sorted.size () < max_blocks)
	{
		// If count of tally items is less than 10, remove any block without tally
		for (auto const & [hash, block] : blocks ())
		{
			if (std::find_if (sorted.begin (), sorted.end (), [&hash = hash] (auto const & item_a) { return item_a.first == hash; }) == sorted.end () && hash != winner_hash)
			{
				replaced_block = hash;
				break;
			}
		}
	}
	else if (inactive_tally > 0 && inactive_tally > sorted.front ().second)
	{
		if (sorted.front ().first != winner_hash)
		{
			replaced_block = sorted.front ().first;
		}
		else if (inactive_tally > sorted[1].second)
		{
			// Avoid removing winner
			replaced_block = sorted[1].first;
		}
	}

	bool replaced (false);
	if (!replaced_block.is_zero ())
	{
		node.active.erase_hash (replaced_block);
		lock_a.lock ();
		remove_block (replaced_block);
		replaced = true;
	}
	else
	{
		lock_a.lock ();
	}
	return replaced;
}

void vxldollar::election::force_confirm (vxldollar::election_status_type type_a)
{
	release_assert (node.network_params.network.is_dev_network ());
	vxldollar::unique_lock<vxldollar::mutex> lock (mutex);
	confirm_once (lock, type_a);
}

std::unordered_map<vxldollar::block_hash, std::shared_ptr<vxldollar::block>> vxldollar::election::blocks () const
{
	vxldollar::lock_guard<vxldollar::mutex> guard (mutex);
	return last_blocks;
}

std::unordered_map<vxldollar::account, vxldollar::vote_info> vxldollar::election::votes () const
{
	vxldollar::lock_guard<vxldollar::mutex> guard (mutex);
	return last_votes;
}

std::vector<vxldollar::vote_with_weight_info> vxldollar::election::votes_with_weight () const
{
	std::multimap<vxldollar::uint128_t, vxldollar::vote_with_weight_info, std::greater<vxldollar::uint128_t>> sorted_votes;
	std::vector<vxldollar::vote_with_weight_info> result;
	auto votes_l (votes ());
	for (auto const & vote_l : votes_l)
	{
		if (vote_l.first != nullptr)
		{
			auto amount (node.ledger.cache.rep_weights.representation_get (vote_l.first));
			vxldollar::vote_with_weight_info vote_info{ vote_l.first, vote_l.second.time, vote_l.second.timestamp, vote_l.second.hash, amount };
			sorted_votes.emplace (std::move (amount), vote_info);
		}
	}
	result.reserve (sorted_votes.size ());
	std::transform (sorted_votes.begin (), sorted_votes.end (), std::back_inserter (result), [] (auto const & entry) { return entry.second; });
	return result;
}
