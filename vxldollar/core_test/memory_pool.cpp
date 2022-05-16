#include <vxldollar/lib/memory.hpp>
#include <vxldollar/node/active_transactions.hpp>
#include <vxldollar/secure/common.hpp>

#include <gtest/gtest.h>

#include <memory>
#include <vector>

namespace
{
/** This allocator records the size of all allocations that happen */
template <class T>
class record_allocations_new_delete_allocator
{
public:
	using value_type = T;

	explicit record_allocations_new_delete_allocator (std::vector<size_t> * allocated) :
		allocated (allocated)
	{
	}

	template <typename U>
	record_allocations_new_delete_allocator (const record_allocations_new_delete_allocator<U> & a)
	{
		allocated = a.allocated;
	}

	template <typename U>
	record_allocations_new_delete_allocator & operator= (const record_allocations_new_delete_allocator<U> &) = delete;

	T * allocate (size_t num_to_allocate)
	{
		auto size_allocated = (sizeof (T) * num_to_allocate);
		allocated->push_back (size_allocated);
		return static_cast<T *> (::operator new (size_allocated));
	}

	void deallocate (T * p, size_t num_to_deallocate)
	{
		::operator delete (p);
	}

	std::vector<size_t> * allocated;
};

template <typename T>
size_t get_allocated_size ()
{
	std::vector<size_t> allocated;
	record_allocations_new_delete_allocator<T> alloc (&allocated);
	(void)std::allocate_shared<T, record_allocations_new_delete_allocator<T>> (alloc);
	debug_assert (allocated.size () == 1);
	return allocated.front ();
}
}

TEST (memory_pool, validate_cleanup)
{
	// This might be turned off, e.g on Mac for instance, so don't do this test
	if (!vxldollar::get_use_memory_pools ())
	{
		return;
	}

	vxldollar::make_shared<vxldollar::open_block> ();
	vxldollar::make_shared<vxldollar::receive_block> ();
	vxldollar::make_shared<vxldollar::send_block> ();
	vxldollar::make_shared<vxldollar::change_block> ();
	vxldollar::make_shared<vxldollar::state_block> ();
	vxldollar::make_shared<vxldollar::vote> ();

	ASSERT_TRUE (vxldollar::purge_shared_ptr_singleton_pool_memory<vxldollar::open_block> ());
	ASSERT_TRUE (vxldollar::purge_shared_ptr_singleton_pool_memory<vxldollar::receive_block> ());
	ASSERT_TRUE (vxldollar::purge_shared_ptr_singleton_pool_memory<vxldollar::send_block> ());
	ASSERT_TRUE (vxldollar::purge_shared_ptr_singleton_pool_memory<vxldollar::state_block> ());
	ASSERT_TRUE (vxldollar::purge_shared_ptr_singleton_pool_memory<vxldollar::vote> ());

	// Change blocks have the same size as open_block so won't deallocate any memory
	ASSERT_FALSE (vxldollar::purge_shared_ptr_singleton_pool_memory<vxldollar::change_block> ());

	ASSERT_EQ (vxldollar::determine_shared_ptr_pool_size<vxldollar::open_block> (), get_allocated_size<vxldollar::open_block> () - sizeof (size_t));
	ASSERT_EQ (vxldollar::determine_shared_ptr_pool_size<vxldollar::receive_block> (), get_allocated_size<vxldollar::receive_block> () - sizeof (size_t));
	ASSERT_EQ (vxldollar::determine_shared_ptr_pool_size<vxldollar::send_block> (), get_allocated_size<vxldollar::send_block> () - sizeof (size_t));
	ASSERT_EQ (vxldollar::determine_shared_ptr_pool_size<vxldollar::change_block> (), get_allocated_size<vxldollar::change_block> () - sizeof (size_t));
	ASSERT_EQ (vxldollar::determine_shared_ptr_pool_size<vxldollar::state_block> (), get_allocated_size<vxldollar::state_block> () - sizeof (size_t));
	ASSERT_EQ (vxldollar::determine_shared_ptr_pool_size<vxldollar::vote> (), get_allocated_size<vxldollar::vote> () - sizeof (size_t));

	{
		vxldollar::active_transactions::ordered_cache inactive_votes_cache;
		vxldollar::account representative{ 1 };
		vxldollar::block_hash hash{ 1 };
		uint64_t timestamp{ 1 };
		vxldollar::inactive_cache_status default_status{};
		inactive_votes_cache.emplace (std::chrono::steady_clock::now (), hash, representative, timestamp, default_status);
	}

	ASSERT_TRUE (vxldollar::purge_singleton_inactive_votes_cache_pool_memory ());
}
