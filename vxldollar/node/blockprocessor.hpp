#pragma once

#include <vxldollar/lib/blocks.hpp>
#include <vxldollar/node/state_block_signature_verification.hpp>
#include <vxldollar/secure/common.hpp>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>

#include <chrono>
#include <memory>
#include <thread>
#include <unordered_set>

namespace vxldollar
{
class node;
class read_transaction;
class transaction;
class write_transaction;
class write_database_queue;

enum class block_origin
{
	local,
	remote
};

class block_post_events final
{
public:
	explicit block_post_events (std::function<vxldollar::read_transaction ()> &&);
	~block_post_events ();
	std::deque<std::function<void (vxldollar::read_transaction const &)>> events;

private:
	std::function<vxldollar::read_transaction ()> get_transaction;
};

/**
 * Processing blocks is a potentially long IO operation.
 * This class isolates block insertion from other operations like servicing network operations
 */
class block_processor final
{
public:
	explicit block_processor (vxldollar::node &, vxldollar::write_database_queue &);
	~block_processor ();
	void stop ();
	void flush ();
	std::size_t size ();
	bool full ();
	bool half_full ();
	void add_local (vxldollar::unchecked_info const & info_a);
	void add (vxldollar::unchecked_info const &);
	void add (std::shared_ptr<vxldollar::block> const &);
	void force (std::shared_ptr<vxldollar::block> const &);
	void wait_write ();
	bool should_log ();
	bool have_blocks_ready ();
	bool have_blocks ();
	void process_blocks ();
	vxldollar::process_return process_one (vxldollar::write_transaction const &, block_post_events &, vxldollar::unchecked_info, bool const = false, vxldollar::block_origin const = vxldollar::block_origin::remote);
	vxldollar::process_return process_one (vxldollar::write_transaction const &, block_post_events &, std::shared_ptr<vxldollar::block> const &);
	std::atomic<bool> flushing{ false };
	// Delay required for average network propagartion before requesting confirmation
	static std::chrono::milliseconds constexpr confirmation_request_delay{ 1500 };

private:
	void queue_unchecked (vxldollar::write_transaction const &, vxldollar::hash_or_account const &);
	void process_batch (vxldollar::unique_lock<vxldollar::mutex> &);
	void process_live (vxldollar::transaction const &, vxldollar::block_hash const &, std::shared_ptr<vxldollar::block> const &, vxldollar::process_return const &, vxldollar::block_origin const = vxldollar::block_origin::remote);
	void requeue_invalid (vxldollar::block_hash const &, vxldollar::unchecked_info const &);
	void process_verified_state_blocks (std::deque<vxldollar::state_block_signature_verification::value_type> &, std::vector<int> const &, std::vector<vxldollar::block_hash> const &, std::vector<vxldollar::signature> const &);
	bool stopped{ false };
	bool active{ false };
	bool awaiting_write{ false };
	std::chrono::steady_clock::time_point next_log;
	std::deque<vxldollar::unchecked_info> blocks;
	std::deque<std::shared_ptr<vxldollar::block>> forced;
	vxldollar::condition_variable condition;
	vxldollar::node & node;
	vxldollar::write_database_queue & write_database_queue;
	vxldollar::mutex mutex{ mutex_identifier (mutexes::block_processor) };
	vxldollar::state_block_signature_verification state_block_signature_verification;
	std::thread processing_thread;

	friend std::unique_ptr<container_info_component> collect_container_info (block_processor & block_processor, std::string const & name);
};
std::unique_ptr<vxldollar::container_info_component> collect_container_info (block_processor & block_processor, std::string const & name);
}
