#pragma once

#include <vxldollar/secure/store_partial.hpp>

namespace
{
template <typename T>
void parallel_traversal (std::function<void (T const &, T const &, bool const)> const & action);
}

namespace vxldollar
{
template <typename Val, typename Derived_Store>
class store_partial;

template <typename Val, typename Derived_Store>
void release_assert_success (store_partial<Val, Derived_Store> const &, int const);

template <typename Val, typename Derived_Store>
class pruned_store_partial : public pruned_store
{
private:
	vxldollar::store_partial<Val, Derived_Store> & store;

	friend void release_assert_success<Val, Derived_Store> (store_partial<Val, Derived_Store> const &, int const);

public:
	explicit pruned_store_partial (vxldollar::store_partial<Val, Derived_Store> & store_a) :
		store (store_a){};

	void put (vxldollar::write_transaction const & transaction_a, vxldollar::block_hash const & hash_a) override
	{
		auto status = store.put_key (transaction_a, tables::pruned, hash_a);
		release_assert_success (store, status);
	}

	void del (vxldollar::write_transaction const & transaction_a, vxldollar::block_hash const & hash_a) override
	{
		auto status = store.del (transaction_a, tables::pruned, hash_a);
		release_assert_success (store, status);
	}

	bool exists (vxldollar::transaction const & transaction_a, vxldollar::block_hash const & hash_a) const override
	{
		return store.exists (transaction_a, tables::pruned, vxldollar::db_val<Val> (hash_a));
	}

	vxldollar::block_hash random (vxldollar::transaction const & transaction_a) override
	{
		vxldollar::block_hash random_hash;
		vxldollar::random_pool::generate_block (random_hash.bytes.data (), random_hash.bytes.size ());
		auto existing = store.template make_iterator<vxldollar::block_hash, vxldollar::db_val<Val>> (transaction_a, tables::pruned, vxldollar::db_val<Val> (random_hash));
		auto end (vxldollar::store_iterator<vxldollar::block_hash, vxldollar::db_val<Val>> (nullptr));
		if (existing == end)
		{
			existing = store.template make_iterator<vxldollar::block_hash, vxldollar::db_val<Val>> (transaction_a, tables::pruned);
		}
		return existing != end ? existing->first : 0;
	}

	size_t count (vxldollar::transaction const & transaction_a) const override
	{
		return store.count (transaction_a, tables::pruned);
	}

	void clear (vxldollar::write_transaction const & transaction_a) override
	{
		auto status = store.drop (transaction_a, tables::pruned);
		release_assert_success (store, status);
	}

	vxldollar::store_iterator<vxldollar::block_hash, std::nullptr_t> begin (vxldollar::transaction const & transaction_a, vxldollar::block_hash const & hash_a) const override
	{
		return store.template make_iterator<vxldollar::block_hash, std::nullptr_t> (transaction_a, tables::pruned, vxldollar::db_val<Val> (hash_a));
	}

	vxldollar::store_iterator<vxldollar::block_hash, std::nullptr_t> begin (vxldollar::transaction const & transaction_a) const override
	{
		return store.template make_iterator<vxldollar::block_hash, std::nullptr_t> (transaction_a, tables::pruned);
	}

	vxldollar::store_iterator<vxldollar::block_hash, std::nullptr_t> end () const override
	{
		return vxldollar::store_iterator<vxldollar::block_hash, std::nullptr_t> (nullptr);
	}

	void for_each_par (std::function<void (vxldollar::read_transaction const &, vxldollar::store_iterator<vxldollar::block_hash, std::nullptr_t>, vxldollar::store_iterator<vxldollar::block_hash, std::nullptr_t>)> const & action_a) const override
	{
		parallel_traversal<vxldollar::uint256_t> (
		[&action_a, this] (vxldollar::uint256_t const & start, vxldollar::uint256_t const & end, bool const is_last) {
			auto transaction (this->store.tx_begin_read ());
			action_a (transaction, this->begin (transaction, start), !is_last ? this->begin (transaction, end) : this->end ());
		});
	}
};

}
