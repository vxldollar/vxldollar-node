#include <vxldollar/node/election_scheduler.hpp>
#include <vxldollar/node/node.hpp>

vxldollar::election_scheduler::election_scheduler (vxldollar::node & node) :
	node{ node },
	stopped{ false },
	thread{ [this] () { run (); } }
{
}

vxldollar::election_scheduler::~election_scheduler ()
{
	stop ();
	thread.join ();
}

void vxldollar::election_scheduler::manual (std::shared_ptr<vxldollar::block> const & block_a, boost::optional<vxldollar::uint128_t> const & previous_balance_a, vxldollar::election_behavior election_behavior_a, std::function<void (std::shared_ptr<vxldollar::block> const &)> const & confirmation_action_a)
{
	vxldollar::lock_guard<vxldollar::mutex> lock{ mutex };
	manual_queue.push_back (std::make_tuple (block_a, previous_balance_a, election_behavior_a, confirmation_action_a));
	notify ();
}

void vxldollar::election_scheduler::activate (vxldollar::account const & account_a, vxldollar::transaction const & transaction)
{
	debug_assert (!account_a.is_zero ());
	vxldollar::account_info account_info;
	if (!node.store.account.get (transaction, account_a, account_info))
	{
		vxldollar::confirmation_height_info conf_info;
		node.store.confirmation_height.get (transaction, account_a, conf_info);
		if (conf_info.height < account_info.block_count)
		{
			debug_assert (conf_info.frontier != account_info.head);
			auto hash = conf_info.height == 0 ? account_info.open_block : node.store.block.successor (transaction, conf_info.frontier);
			auto block = node.store.block.get (transaction, hash);
			debug_assert (block != nullptr);
			if (node.ledger.dependents_confirmed (transaction, *block))
			{
				vxldollar::lock_guard<vxldollar::mutex> lock{ mutex };
				priority.push (account_info.modified, block);
				notify ();
			}
		}
	}
}

void vxldollar::election_scheduler::stop ()
{
	vxldollar::unique_lock<vxldollar::mutex> lock{ mutex };
	stopped = true;
	notify ();
}

void vxldollar::election_scheduler::flush ()
{
	vxldollar::unique_lock<vxldollar::mutex> lock{ mutex };
	condition.wait (lock, [this] () {
		return stopped || empty_locked () || node.active.vacancy () <= 0;
	});
}

void vxldollar::election_scheduler::notify ()
{
	condition.notify_all ();
}

std::size_t vxldollar::election_scheduler::size () const
{
	vxldollar::lock_guard<vxldollar::mutex> lock{ mutex };
	return priority.size () + manual_queue.size ();
}

bool vxldollar::election_scheduler::empty_locked () const
{
	return priority.empty () && manual_queue.empty ();
}

bool vxldollar::election_scheduler::empty () const
{
	vxldollar::lock_guard<vxldollar::mutex> lock{ mutex };
	return empty_locked ();
}

std::size_t vxldollar::election_scheduler::priority_queue_size () const
{
	return priority.size ();
}

bool vxldollar::election_scheduler::priority_queue_predicate () const
{
	return node.active.vacancy () > 0 && !priority.empty ();
}

bool vxldollar::election_scheduler::manual_queue_predicate () const
{
	return !manual_queue.empty ();
}

bool vxldollar::election_scheduler::overfill_predicate () const
{
	return node.active.vacancy () < 0;
}

void vxldollar::election_scheduler::run ()
{
	vxldollar::thread_role::set (vxldollar::thread_role::name::election_scheduler);
	vxldollar::unique_lock<vxldollar::mutex> lock{ mutex };
	while (!stopped)
	{
		condition.wait (lock, [this] () {
			return stopped || priority_queue_predicate () || manual_queue_predicate () || overfill_predicate ();
		});
		debug_assert ((std::this_thread::yield (), true)); // Introduce some random delay in debug builds
		if (!stopped)
		{
			if (overfill_predicate ())
			{
				lock.unlock ();
				node.active.erase_oldest ();
			}
			else if (manual_queue_predicate ())
			{
				auto const [block, previous_balance, election_behavior, confirmation_action] = manual_queue.front ();
				manual_queue.pop_front ();
				lock.unlock ();
				vxldollar::unique_lock<vxldollar::mutex> lock2 (node.active.mutex);
				node.active.insert_impl (lock2, block, previous_balance, election_behavior, confirmation_action);
			}
			else if (priority_queue_predicate ())
			{
				auto block = priority.top ();
				priority.pop ();
				lock.unlock ();
				std::shared_ptr<vxldollar::election> election;
				vxldollar::unique_lock<vxldollar::mutex> lock2 (node.active.mutex);
				election = node.active.insert_impl (lock2, block).election;
				if (election != nullptr)
				{
					election->transition_active ();
				}
			}
			else
			{
				lock.unlock ();
			}
			notify ();
			lock.lock ();
		}
	}
}

std::unique_ptr<vxldollar::container_info_component> vxldollar::election_scheduler::collect_container_info (std::string const & name)
{
	vxldollar::unique_lock<vxldollar::mutex> lock{ mutex };

	auto composite = std::make_unique<container_info_composite> (name);
	composite->add_component (std::make_unique<container_info_leaf> (container_info{ "manual_queue", manual_queue.size (), sizeof (decltype (manual_queue)::value_type) }));
	composite->add_component (priority.collect_container_info ("priority"));
	return composite;
}