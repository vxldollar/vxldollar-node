#pragma once

#include <vxldollar/lib/config.hpp>
#include <vxldollar/lib/locks.hpp>
#include <vxldollar/lib/numbers.hpp>
#include <vxldollar/lib/utility.hpp>

#include <boost/optional.hpp>
#include <boost/thread/thread.hpp>

#include <atomic>
#include <memory>

namespace vxldollar
{
std::string to_string (vxldollar::work_version const version_a);

class block;
class block_details;
enum class block_type : uint8_t;

class opencl_work;
class work_item final
{
public:
	work_item (vxldollar::work_version const version_a, vxldollar::root const & item_a, uint64_t difficulty_a, std::function<void (boost::optional<uint64_t> const &)> const & callback_a) :
		version (version_a), item (item_a), difficulty (difficulty_a), callback (callback_a)
	{
	}
	vxldollar::work_version const version;
	vxldollar::root const item;
	uint64_t const difficulty;
	std::function<void (boost::optional<uint64_t> const &)> const callback;
};
class work_pool final
{
public:
	work_pool (vxldollar::network_constants & network_constants, unsigned, std::chrono::nanoseconds = std::chrono::nanoseconds (0), std::function<boost::optional<uint64_t> (vxldollar::work_version const, vxldollar::root const &, uint64_t, std::atomic<int> &)> = nullptr);
	~work_pool ();
	void loop (uint64_t);
	void stop ();
	void cancel (vxldollar::root const &);
	void generate (vxldollar::work_version const, vxldollar::root const &, uint64_t, std::function<void (boost::optional<uint64_t> const &)>);
	boost::optional<uint64_t> generate (vxldollar::work_version const, vxldollar::root const &, uint64_t);
	// For tests only
	boost::optional<uint64_t> generate (vxldollar::root const &);
	boost::optional<uint64_t> generate (vxldollar::root const &, uint64_t);
	size_t size ();
	vxldollar::network_constants & network_constants;
	std::atomic<int> ticket;
	bool done;
	std::vector<boost::thread> threads;
	std::list<vxldollar::work_item> pending;
	vxldollar::mutex mutex{ mutex_identifier (mutexes::work_pool) };
	vxldollar::condition_variable producer_condition;
	std::chrono::nanoseconds pow_rate_limiter;
	std::function<boost::optional<uint64_t> (vxldollar::work_version const, vxldollar::root const &, uint64_t, std::atomic<int> &)> opencl;
	vxldollar::observer_set<bool> work_observers;
};

std::unique_ptr<container_info_component> collect_container_info (work_pool & work_pool, std::string const & name);
}
