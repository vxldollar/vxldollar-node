#pragma once

#include <vxldollar/lib/numbers.hpp>
#include <vxldollar/lib/utility.hpp>
#include <vxldollar/node/active_transactions.hpp>
#include <vxldollar/node/transport/transport.hpp>

namespace vxldollar
{
class telemetry;
class node_observers final
{
public:
	using blocks_t = vxldollar::observer_set<vxldollar::election_status const &, std::vector<vxldollar::vote_with_weight_info> const &, vxldollar::account const &, vxldollar::uint128_t const &, bool, bool>;
	blocks_t blocks;
	vxldollar::observer_set<bool> wallet;
	vxldollar::observer_set<std::shared_ptr<vxldollar::vote>, std::shared_ptr<vxldollar::transport::channel>, vxldollar::vote_code> vote;
	vxldollar::observer_set<vxldollar::block_hash const &> active_stopped;
	vxldollar::observer_set<vxldollar::account const &, bool> account_balance;
	vxldollar::observer_set<std::shared_ptr<vxldollar::transport::channel>> endpoint;
	vxldollar::observer_set<> disconnect;
	vxldollar::observer_set<vxldollar::root const &> work_cancel;
	vxldollar::observer_set<vxldollar::telemetry_data const &, vxldollar::endpoint const &> telemetry;
};

std::unique_ptr<container_info_component> collect_container_info (node_observers & node_observers, std::string const & name);
}
