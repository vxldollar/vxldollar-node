#include <vxldollar/node/nodeconfig.hpp>
#include <vxldollar/node/online_reps.hpp>
#include <vxldollar/secure/ledger.hpp>
#include <vxldollar/secure/store.hpp>

vxldollar::online_reps::online_reps (vxldollar::ledger & ledger_a, vxldollar::node_config const & config_a) :
	ledger{ ledger_a },
	config{ config_a }
{
	if (!ledger.store.init_error ())
	{
		auto transaction (ledger.store.tx_begin_read ());
		trended_m = calculate_trend (transaction);
	}
}

void vxldollar::online_reps::observe (vxldollar::account const & rep_a)
{
	if (ledger.weight (rep_a) > 0)
	{
		vxldollar::lock_guard<vxldollar::mutex> lock (mutex);
		auto now = std::chrono::steady_clock::now ();
		auto new_insert = reps.get<tag_account> ().erase (rep_a) == 0;
		reps.insert ({ now, rep_a });
		auto cutoff = reps.get<tag_time> ().lower_bound (now - std::chrono::seconds (config.network_params.node.weight_period));
		auto trimmed = reps.get<tag_time> ().begin () != cutoff;
		reps.get<tag_time> ().erase (reps.get<tag_time> ().begin (), cutoff);
		if (new_insert || trimmed)
		{
			online_m = calculate_online ();
		}
	}
}

void vxldollar::online_reps::sample ()
{
	vxldollar::unique_lock<vxldollar::mutex> lock (mutex);
	vxldollar::uint128_t online_l = online_m;
	lock.unlock ();
	vxldollar::uint128_t trend_l;
	{
		auto transaction (ledger.store.tx_begin_write ({ tables::online_weight }));
		// Discard oldest entries
		while (ledger.store.online_weight.count (transaction) >= config.network_params.node.max_weight_samples)
		{
			auto oldest (ledger.store.online_weight.begin (transaction));
			debug_assert (oldest != ledger.store.online_weight.end ());
			ledger.store.online_weight.del (transaction, oldest->first);
		}
		ledger.store.online_weight.put (transaction, std::chrono::system_clock::now ().time_since_epoch ().count (), online_l);
		trend_l = calculate_trend (transaction);
	}
	lock.lock ();
	trended_m = trend_l;
}

vxldollar::uint128_t vxldollar::online_reps::calculate_online () const
{
	vxldollar::uint128_t current;
	for (auto & i : reps)
	{
		current += ledger.weight (i.account);
	}
	return current;
}

vxldollar::uint128_t vxldollar::online_reps::calculate_trend (vxldollar::transaction & transaction_a) const
{
	std::vector<vxldollar::uint128_t> items;
	items.reserve (config.network_params.node.max_weight_samples + 1);
	items.push_back (config.online_weight_minimum.number ());
	for (auto i (ledger.store.online_weight.begin (transaction_a)), n (ledger.store.online_weight.end ()); i != n; ++i)
	{
		items.push_back (i->second.number ());
	}
	vxldollar::uint128_t result;
	// Pick median value for our target vote weight
	auto median_idx = items.size () / 2;
	nth_element (items.begin (), items.begin () + median_idx, items.end ());
	result = items[median_idx];
	return result;
}

vxldollar::uint128_t vxldollar::online_reps::trended () const
{
	vxldollar::lock_guard<vxldollar::mutex> lock (mutex);
	return trended_m;
}

vxldollar::uint128_t vxldollar::online_reps::online () const
{
	vxldollar::lock_guard<vxldollar::mutex> lock (mutex);
	return online_m;
}

vxldollar::uint128_t vxldollar::online_reps::delta () const
{
	vxldollar::lock_guard<vxldollar::mutex> lock (mutex);
	// Using a larger container to ensure maximum precision
	auto weight = static_cast<vxldollar::uint256_t> (std::max ({ online_m, trended_m, config.online_weight_minimum.number () }));
	return ((weight * online_weight_quorum) / 100).convert_to<vxldollar::uint128_t> ();
}

std::vector<vxldollar::account> vxldollar::online_reps::list ()
{
	std::vector<vxldollar::account> result;
	vxldollar::lock_guard<vxldollar::mutex> lock (mutex);
	std::for_each (reps.begin (), reps.end (), [&result] (rep_info const & info_a) { result.push_back (info_a.account); });
	return result;
}

void vxldollar::online_reps::clear ()
{
	vxldollar::lock_guard<vxldollar::mutex> lock (mutex);
	reps.clear ();
	online_m = 0;
}

std::unique_ptr<vxldollar::container_info_component> vxldollar::collect_container_info (online_reps & online_reps, std::string const & name)
{
	std::size_t count;
	{
		vxldollar::lock_guard<vxldollar::mutex> guard (online_reps.mutex);
		count = online_reps.reps.size ();
	}

	auto sizeof_element = sizeof (decltype (online_reps.reps)::value_type);
	auto composite = std::make_unique<container_info_composite> (name);
	composite->add_component (std::make_unique<container_info_leaf> (container_info{ "reps", count, sizeof_element }));
	return composite;
}
