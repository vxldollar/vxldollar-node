#include <vxldollar/lib/locks.hpp>
#include <vxldollar/lib/threading.hpp>
#include <vxldollar/lib/timer.hpp>
#include <vxldollar/node/unchecked_map.hpp>
#include <vxldollar/secure/store.hpp>

#include <boost/range/join.hpp>

vxldollar::unchecked_map::unchecked_map (vxldollar::store & store, bool const & disable_delete) :
	store{ store },
	disable_delete{ disable_delete },
	thread{ [this] () { run (); } }
{
}

vxldollar::unchecked_map::~unchecked_map ()
{
	stop ();
	thread.join ();
}

void vxldollar::unchecked_map::put (vxldollar::hash_or_account const & dependency, vxldollar::unchecked_info const & info)
{
	vxldollar::unique_lock<vxldollar::mutex> lock{ mutex };
	buffer.push_back (std::make_pair (dependency, info));
	lock.unlock ();
	condition.notify_all (); // Notify run ()
}

auto vxldollar::unchecked_map::equal_range (vxldollar::transaction const & transaction, vxldollar::block_hash const & dependency) -> std::pair<iterator, iterator>
{
	return store.unchecked.equal_range (transaction, dependency);
}

auto vxldollar::unchecked_map::full_range (vxldollar::transaction const & transaction) -> std::pair<iterator, iterator>
{
	return store.unchecked.full_range (transaction);
}

std::vector<vxldollar::unchecked_info> vxldollar::unchecked_map::get (vxldollar::transaction const & transaction, vxldollar::block_hash const & hash)
{
	return store.unchecked.get (transaction, hash);
}

bool vxldollar::unchecked_map::exists (vxldollar::transaction const & transaction, vxldollar::unchecked_key const & key) const
{
	return store.unchecked.exists (transaction, key);
}

void vxldollar::unchecked_map::del (vxldollar::write_transaction const & transaction, vxldollar::unchecked_key const & key)
{
	store.unchecked.del (transaction, key);
}

void vxldollar::unchecked_map::clear (vxldollar::write_transaction const & transaction)
{
	store.unchecked.clear (transaction);
}

size_t vxldollar::unchecked_map::count (vxldollar::transaction const & transaction) const
{
	return store.unchecked.count (transaction);
}

void vxldollar::unchecked_map::stop ()
{
	vxldollar::unique_lock<vxldollar::mutex> lock{ mutex };
	if (!stopped)
	{
		stopped = true;
		condition.notify_all (); // Notify flush (), run ()
	}
}

void vxldollar::unchecked_map::flush ()
{
	vxldollar::unique_lock<vxldollar::mutex> lock{ mutex };
	condition.wait (lock, [this] () {
		return stopped || (buffer.empty () && back_buffer.empty () && !writing_back_buffer);
	});
}

void vxldollar::unchecked_map::trigger (vxldollar::hash_or_account const & dependency)
{
	vxldollar::unique_lock<vxldollar::mutex> lock{ mutex };
	buffer.push_back (dependency);
	debug_assert (buffer.back ().which () == 1); // which stands for "query".
	lock.unlock ();
	condition.notify_all (); // Notify run ()
}

vxldollar::unchecked_map::item_visitor::item_visitor (unchecked_map & unchecked, vxldollar::write_transaction const & transaction) :
	unchecked{ unchecked },
	transaction{ transaction }
{
}
void vxldollar::unchecked_map::item_visitor::operator() (insert const & item)
{
	auto const & [dependency, info] = item;
	unchecked.store.unchecked.put (transaction, dependency, { info.block, info.account, info.verified });
}

void vxldollar::unchecked_map::item_visitor::operator() (query const & item)
{
	auto [i, n] = unchecked.store.unchecked.equal_range (transaction, item.hash);
	std::deque<vxldollar::unchecked_key> delete_queue;
	for (; i != n; ++i)
	{
		auto const & key = i->first;
		auto const & info = i->second;
		delete_queue.push_back (key);
		unchecked.satisfied (info);
	}
	if (!unchecked.disable_delete)
	{
		for (auto const & key : delete_queue)
		{
			unchecked.del (transaction, key);
		}
	}
}

void vxldollar::unchecked_map::write_buffer (decltype (buffer) const & back_buffer)
{
	auto transaction = store.tx_begin_write ();
	item_visitor visitor{ *this, transaction };
	for (auto const & item : back_buffer)
	{
		boost::apply_visitor (visitor, item);
	}
}

void vxldollar::unchecked_map::run ()
{
	vxldollar::thread_role::set (vxldollar::thread_role::name::unchecked);
	vxldollar::unique_lock<vxldollar::mutex> lock{ mutex };
	while (!stopped)
	{
		if (!buffer.empty ())
		{
			back_buffer.swap (buffer);
			writing_back_buffer = true;
			lock.unlock ();
			write_buffer (back_buffer);
			lock.lock ();
			writing_back_buffer = false;
			back_buffer.clear ();
		}
		else
		{
			condition.notify_all (); // Notify flush ()
			condition.wait (lock, [this] () {
				return stopped || !buffer.empty ();
			});
		}
	}
}
