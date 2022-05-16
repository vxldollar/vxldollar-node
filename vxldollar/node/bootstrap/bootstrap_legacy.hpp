#pragma once

#include <vxldollar/node/bootstrap/bootstrap_attempt.hpp>

#include <boost/property_tree/ptree_fwd.hpp>

#include <atomic>
#include <deque>
#include <memory>
#include <vector>

namespace vxldollar
{
class node;

/**
 * Legacy bootstrap session. This is made up of 3 phases: frontier requests, bootstrap pulls, bootstrap pushes.
 */
class bootstrap_attempt_legacy : public bootstrap_attempt
{
public:
	explicit bootstrap_attempt_legacy (std::shared_ptr<vxldollar::node> const & node_a, uint64_t const incremental_id_a, std::string const & id_a, uint32_t const frontiers_age_a, vxldollar::account const & start_account_a);
	void run () override;
	bool consume_future (std::future<bool> &);
	void stop () override;
	bool request_frontier (vxldollar::unique_lock<vxldollar::mutex> &, bool = false);
	void request_push (vxldollar::unique_lock<vxldollar::mutex> &);
	void add_frontier (vxldollar::pull_info const &) override;
	void add_bulk_push_target (vxldollar::block_hash const &, vxldollar::block_hash const &) override;
	bool request_bulk_push_target (std::pair<vxldollar::block_hash, vxldollar::block_hash> &) override;
	void set_start_account (vxldollar::account const &) override;
	void run_start (vxldollar::unique_lock<vxldollar::mutex> &);
	void get_information (boost::property_tree::ptree &) override;
	vxldollar::tcp_endpoint endpoint_frontier_request;
	std::weak_ptr<vxldollar::frontier_req_client> frontiers;
	std::weak_ptr<vxldollar::bulk_push_client> push;
	std::deque<vxldollar::pull_info> frontier_pulls;
	std::vector<std::pair<vxldollar::block_hash, vxldollar::block_hash>> bulk_push_targets;
	vxldollar::account start_account{};
	std::atomic<unsigned> account_count{ 0 };
	uint32_t frontiers_age;
};
}
