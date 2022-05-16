#include <vxldollar/lib/config.hpp>
#include <vxldollar/lib/utility.hpp>
#include <vxldollar/node/write_database_queue.hpp>

#include <algorithm>

vxldollar::write_guard::write_guard (std::function<void ()> guard_finish_callback_a) :
	guard_finish_callback (guard_finish_callback_a)
{
}

vxldollar::write_guard::write_guard (vxldollar::write_guard && write_guard_a) noexcept :
	guard_finish_callback (std::move (write_guard_a.guard_finish_callback)),
	owns (write_guard_a.owns)
{
	write_guard_a.owns = false;
	write_guard_a.guard_finish_callback = nullptr;
}

vxldollar::write_guard & vxldollar::write_guard::operator= (vxldollar::write_guard && write_guard_a) noexcept
{
	owns = write_guard_a.owns;
	guard_finish_callback = std::move (write_guard_a.guard_finish_callback);

	write_guard_a.owns = false;
	write_guard_a.guard_finish_callback = nullptr;
	return *this;
}

vxldollar::write_guard::~write_guard ()
{
	if (owns)
	{
		guard_finish_callback ();
	}
}

bool vxldollar::write_guard::is_owned () const
{
	return owns;
}

void vxldollar::write_guard::release ()
{
	debug_assert (owns);
	if (owns)
	{
		guard_finish_callback ();
	}
	owns = false;
}

vxldollar::write_database_queue::write_database_queue (bool use_noops_a) :
	guard_finish_callback ([use_noops_a, &queue = queue, &mutex = mutex, &cv = cv] () {
		if (!use_noops_a)
		{
			{
				vxldollar::lock_guard<vxldollar::mutex> guard (mutex);
				queue.pop_front ();
			}
			cv.notify_all ();
		}
	}),
	use_noops (use_noops_a)
{
}

vxldollar::write_guard vxldollar::write_database_queue::wait (vxldollar::writer writer)
{
	if (use_noops)
	{
		return write_guard ([] {});
	}

	vxldollar::unique_lock<vxldollar::mutex> lk (mutex);
	// Add writer to the end of the queue if it's not already waiting
	auto exists = std::find (queue.cbegin (), queue.cend (), writer) != queue.cend ();
	if (!exists)
	{
		queue.push_back (writer);
	}

	while (queue.front () != writer)
	{
		cv.wait (lk);
	}

	return write_guard (guard_finish_callback);
}

bool vxldollar::write_database_queue::contains (vxldollar::writer writer)
{
	debug_assert (!use_noops);
	vxldollar::lock_guard<vxldollar::mutex> guard (mutex);
	return std::find (queue.cbegin (), queue.cend (), writer) != queue.cend ();
}

bool vxldollar::write_database_queue::process (vxldollar::writer writer)
{
	if (use_noops)
	{
		return true;
	}

	auto result = false;
	{
		vxldollar::lock_guard<vxldollar::mutex> guard (mutex);
		// Add writer to the end of the queue if it's not already waiting
		auto exists = std::find (queue.cbegin (), queue.cend (), writer) != queue.cend ();
		if (!exists)
		{
			queue.push_back (writer);
		}

		result = (queue.front () == writer);
	}

	if (!result)
	{
		cv.notify_all ();
	}

	return result;
}

vxldollar::write_guard vxldollar::write_database_queue::pop ()
{
	return write_guard (guard_finish_callback);
}
