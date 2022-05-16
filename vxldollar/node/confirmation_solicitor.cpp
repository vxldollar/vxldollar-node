#include <vxldollar/node/confirmation_solicitor.hpp>
#include <vxldollar/node/election.hpp>
#include <vxldollar/node/nodeconfig.hpp>

using namespace std::chrono_literals;

vxldollar::confirmation_solicitor::confirmation_solicitor (vxldollar::network & network_a, vxldollar::node_config const & config_a) :
	max_block_broadcasts (config_a.network_params.network.is_dev_network () ? 4 : 30),
	max_election_requests (50),
	max_election_broadcasts (std::max<std::size_t> (network_a.fanout () / 2, 1)),
	network (network_a),
	config (config_a)
{
}

void vxldollar::confirmation_solicitor::prepare (std::vector<vxldollar::representative> const & representatives_a)
{
	debug_assert (!prepared);
	requests.clear ();
	rebroadcasted = 0;
	/** Two copies are required as representatives can be erased from \p representatives_requests */
	representatives_requests = representatives_a;
	representatives_broadcasts = representatives_a;
	prepared = true;
}

bool vxldollar::confirmation_solicitor::broadcast (vxldollar::election const & election_a)
{
	debug_assert (prepared);
	bool error (true);
	if (rebroadcasted++ < max_block_broadcasts)
	{
		auto const & hash (election_a.status.winner->hash ());
		vxldollar::publish winner{ config.network_params.network, election_a.status.winner };
		unsigned count = 0;
		// Directed broadcasting to principal representatives
		for (auto i (representatives_broadcasts.begin ()), n (representatives_broadcasts.end ()); i != n && count < max_election_broadcasts; ++i)
		{
			auto existing (election_a.last_votes.find (i->account));
			bool const exists (existing != election_a.last_votes.end ());
			bool const different (exists && existing->second.hash != hash);
			if (!exists || different)
			{
				i->channel->send (winner);
				count += different ? 0 : 1;
			}
		}
		// Random flood for block propagation
		network.flood_message (winner, vxldollar::buffer_drop_policy::limiter, 0.5f);
		error = false;
	}
	return error;
}

bool vxldollar::confirmation_solicitor::add (vxldollar::election const & election_a)
{
	debug_assert (prepared);
	bool error (true);
	unsigned count = 0;
	auto const max_channel_requests (config.confirm_req_batches_max * vxldollar::network::confirm_req_hashes_max);
	auto const & hash (election_a.status.winner->hash ());
	for (auto i (representatives_requests.begin ()); i != representatives_requests.end () && count < max_election_requests;)
	{
		bool full_queue (false);
		auto rep (*i);
		auto existing (election_a.last_votes.find (rep.account));
		bool const exists (existing != election_a.last_votes.end ());
		bool const is_final (exists && (!election_a.is_quorum.load () || existing->second.timestamp == std::numeric_limits<uint64_t>::max ()));
		bool const different (exists && existing->second.hash != hash);
		if (!exists || !is_final || different)
		{
			auto & request_queue (requests[rep.channel]);
			if (request_queue.size () < max_channel_requests)
			{
				request_queue.emplace_back (election_a.status.winner->hash (), election_a.status.winner->root ());
				count += different ? 0 : 1;
				error = false;
			}
			else
			{
				full_queue = true;
			}
		}
		i = !full_queue ? i + 1 : representatives_requests.erase (i);
	}
	return error;
}

void vxldollar::confirmation_solicitor::flush ()
{
	debug_assert (prepared);
	for (auto const & request_queue : requests)
	{
		auto const & channel (request_queue.first);
		std::vector<std::pair<vxldollar::block_hash, vxldollar::root>> roots_hashes_l;
		for (auto const & root_hash : request_queue.second)
		{
			roots_hashes_l.push_back (root_hash);
			if (roots_hashes_l.size () == vxldollar::network::confirm_req_hashes_max)
			{
				vxldollar::confirm_req req{ config.network_params.network, roots_hashes_l };
				channel->send (req);
				roots_hashes_l.clear ();
			}
		}
		if (!roots_hashes_l.empty ())
		{
			vxldollar::confirm_req req{ config.network_params.network, roots_hashes_l };
			channel->send (req);
		}
	}
	prepared = false;
}
