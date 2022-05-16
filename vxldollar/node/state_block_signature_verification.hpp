#pragma once

#include <vxldollar/lib/locks.hpp>
#include <vxldollar/secure/common.hpp>

#include <deque>
#include <functional>
#include <thread>

namespace vxldollar
{
class epochs;
class logger_mt;
class node_config;
class signature_checker;

class state_block_signature_verification
{
public:
	using value_type = std::tuple<std::shared_ptr<vxldollar::block>, vxldollar::account, vxldollar::signature_verification>;

	state_block_signature_verification (vxldollar::signature_checker &, vxldollar::epochs &, vxldollar::node_config &, vxldollar::logger_mt &, uint64_t);
	~state_block_signature_verification ();
	void add (value_type const & item);
	std::size_t size ();
	void stop ();
	bool is_active ();

	std::function<void (std::deque<value_type> &, std::vector<int> const &, std::vector<vxldollar::block_hash> const &, std::vector<vxldollar::signature> const &)> blocks_verified_callback;
	std::function<void ()> transition_inactive_callback;

private:
	vxldollar::signature_checker & signature_checker;
	vxldollar::epochs & epochs;
	vxldollar::node_config & node_config;
	vxldollar::logger_mt & logger;

	vxldollar::mutex mutex{ mutex_identifier (mutexes::state_block_signature_verification) };
	bool stopped{ false };
	bool active{ false };
	std::deque<value_type> state_blocks;
	vxldollar::condition_variable condition;
	std::thread thread;

	void run (uint64_t block_processor_verification_size);
	std::deque<value_type> setup_items (std::size_t);
	void verify_state_blocks (std::deque<value_type> &);
};

std::unique_ptr<vxldollar::container_info_component> collect_container_info (state_block_signature_verification & state_block_signature_verification, std::string const & name);
}
