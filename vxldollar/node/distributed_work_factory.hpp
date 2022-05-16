#pragma once

#include <vxldollar/lib/numbers.hpp>

#include <atomic>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace vxldollar
{
class container_info_component;
class distributed_work;
class node;
class root;
struct work_request;

class distributed_work_factory final
{
public:
	distributed_work_factory (vxldollar::node &);
	~distributed_work_factory ();
	bool make (vxldollar::work_version const, vxldollar::root const &, std::vector<std::pair<std::string, uint16_t>> const &, uint64_t, std::function<void (boost::optional<uint64_t>)> const &, boost::optional<vxldollar::account> const & = boost::none);
	bool make (std::chrono::seconds const &, vxldollar::work_request const &);
	void cancel (vxldollar::root const &);
	void cleanup_finished ();
	void stop ();
	std::size_t size () const;

private:
	std::unordered_multimap<vxldollar::root, std::weak_ptr<vxldollar::distributed_work>> items;

	vxldollar::node & node;
	mutable vxldollar::mutex mutex;
	std::atomic<bool> stopped{ false };

	friend std::unique_ptr<container_info_component> collect_container_info (distributed_work_factory &, std::string const &);
};

std::unique_ptr<container_info_component> collect_container_info (distributed_work_factory & distributed_work, std::string const & name);
}
