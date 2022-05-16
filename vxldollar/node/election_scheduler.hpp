#pragma once

#include <vxldollar/lib/numbers.hpp>
#include <vxldollar/node/active_transactions.hpp>
#include <vxldollar/node/prioritization.hpp>

#include <boost/optional.hpp>

#include <condition_variable>
#include <deque>
#include <memory>
#include <thread>

namespace vxldollar
{
class block;
class node;
class election_scheduler final
{
public:
	election_scheduler (vxldollar::node & node);
	~election_scheduler ();
	// Manualy start an election for a block
	// Call action with confirmed block, may be different than what we started with
	void manual (std::shared_ptr<vxldollar::block> const &, boost::optional<vxldollar::uint128_t> const & = boost::none, vxldollar::election_behavior = vxldollar::election_behavior::normal, std::function<void (std::shared_ptr<vxldollar::block> const &)> const & = nullptr);
	// Activates the first unconfirmed block of \p account_a
	void activate (vxldollar::account const &, vxldollar::transaction const &);
	void stop ();
	// Blocks until no more elections can be activated or there are no more elections to activate
	void flush ();
	void notify ();
	std::size_t size () const;
	bool empty () const;
	std::size_t priority_queue_size () const;
	std::unique_ptr<container_info_component> collect_container_info (std::string const &);

private:
	void run ();
	bool empty_locked () const;
	bool priority_queue_predicate () const;
	bool manual_queue_predicate () const;
	bool overfill_predicate () const;
	vxldollar::prioritization priority;
	std::deque<std::tuple<std::shared_ptr<vxldollar::block>, boost::optional<vxldollar::uint128_t>, vxldollar::election_behavior, std::function<void (std::shared_ptr<vxldollar::block>)>>> manual_queue;
	vxldollar::node & node;
	bool stopped;
	vxldollar::condition_variable condition;
	mutable vxldollar::mutex mutex;
	std::thread thread;
};
}