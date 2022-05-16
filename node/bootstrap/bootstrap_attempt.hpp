#pragma once

#include <vxldollar/node/bootstrap/bootstrap.hpp>

#include <atomic>
#include <future>

namespace vxldollar
{
class node;

class frontier_req_client;
class bulk_push_client;

/**
 * Polymorphic base class for bootstrap sessions.
 */
class bootstrap_attempt : public std::enable_shared_from_this<bootstrap_attempt>
{
public:
	explicit bootstrap_attempt (std::shared_ptr<vxldollar::node> const & node_a, vxldollar::bootstrap_mode mode_a, uint64_t incremental_id_a, std::string id_a);
	virtual ~bootstrap_attempt ();
	virtual void run () = 0;
	virtual void stop ();
	bool still_pulling ();
	void pull_started ();
	void pull_finished ();
	bool should_log ();
	std::string mode_text ();
	virtual void add_frontier (vxldollar::pull_info const &);
	virtual void add_bulk_push_target (vxldollar::block_hash const &, vxldollar::block_hash const &);
	virtual bool request_bulk_push_target (std::pair<vxldollar::block_hash, vxldollar::block_hash> &);
	virtual void set_start_account (vxldollar::account const &);
	virtual bool lazy_start (vxldollar::hash_or_account const &, bool confirmed = true);
	virtual void lazy_add (vxldollar::pull_info const &);
	virtual void lazy_requeue (vxldollar::block_hash const &, vxldollar::block_hash const &);
	virtual uint32_t lazy_batch_size ();
	virtual bool lazy_has_expired () const;
	virtual bool lazy_processed_or_exists (vxldollar::block_hash const &);
	virtual bool process_block (std::shared_ptr<vxldollar::block> const &, vxldollar::account const &, uint64_t, vxldollar::bulk_pull::count_t, bool, unsigned);
	virtual void requeue_pending (vxldollar::account const &);
	virtual void wallet_start (std::deque<vxldollar::account> &);
	virtual std::size_t wallet_size ();
	virtual void get_information (boost::property_tree::ptree &) = 0;
	vxldollar::mutex next_log_mutex;
	std::chrono::steady_clock::time_point next_log{ std::chrono::steady_clock::now () };
	std::atomic<unsigned> pulling{ 0 };
	std::shared_ptr<vxldollar::node> node;
	std::atomic<uint64_t> total_blocks{ 0 };
	std::atomic<unsigned> requeued_pulls{ 0 };
	std::atomic<bool> started{ false };
	std::atomic<bool> stopped{ false };
	uint64_t incremental_id{ 0 };
	std::string id;
	std::chrono::steady_clock::time_point attempt_start{ std::chrono::steady_clock::now () };
	std::atomic<bool> frontiers_received{ false };
	vxldollar::bootstrap_mode mode;
	vxldollar::mutex mutex;
	vxldollar::condition_variable condition;
};
}
