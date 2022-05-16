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
void release_assert_success (store_partial<Val, Derived_Store> const & store, int const status);

template <typename Val, typename Derived_Store>
class frontier_store_partial : public frontier_store
{
private:
	vxldollar::store_partial<Val, Derived_Store> & store;

	friend void release_assert_success<Val, Derived_Store> (store_partial<Val, Derived_Store> const &, int const);

public:
	explicit frontier_store_partial (vxldollar::store_partial<Val, Derived_Store> & store_a) :
		store (store_a){};

	void put (vxldollar::write_transaction const & transaction_a, vxldollar::block_hash const & block_a, vxldollar::account const & account_a) override
	{
		vxldollar::db_val<Val> account (account_a);
		auto status (store.put (transaction_a, tables::frontiers, block_a, account));
		release_assert_success (store, status);
	}

	vxldollar::account get (vxldollar::transaction const & transaction_a, vxldollar::block_hash const & block_a) const override
	{
		vxldollar::db_val<Val> value;
		auto status (store.get (transaction_a, tables::frontiers, vxldollar::db_val<Val> (block_a), value));
		release_assert (store.success (status) || store.not_found (status));
		vxldollar::account result{};
		if (store.success (status))
		{
			result = static_cast<vxldollar::account> (value);
		}
		return result;
	}

	void del (vxldollar::write_transaction const & transaction_a, vxldollar::block_hash const & block_a) override
	{
		auto status (store.del (transaction_a, tables::frontiers, block_a));
		release_assert_success (store, status);
	}

	vxldollar::store_iterator<vxldollar::block_hash, vxldollar::account> begin (vxldollar::transaction const & transaction_a) const override
	{
		return store.template make_iterator<vxldollar::block_hash, vxldollar::account> (transaction_a, tables::frontiers);
	}

	vxldollar::store_iterator<vxldollar::block_hash, vxldollar::account> begin (vxldollar::transaction const & transaction_a, vxldollar::block_hash const & hash_a) const override
	{
		return store.template make_iterator<vxldollar::block_hash, vxldollar::account> (transaction_a, tables::frontiers, vxldollar::db_val<Val> (hash_a));
	}

	vxldollar::store_iterator<vxldollar::block_hash, vxldollar::account> end () const override
	{
		return vxldollar::store_iterator<vxldollar::block_hash, vxldollar::account> (nullptr);
	}

	void for_each_par (std::function<void (vxldollar::read_transaction const &, vxldollar::store_iterator<vxldollar::block_hash, vxldollar::account>, vxldollar::store_iterator<vxldollar::block_hash, vxldollar::account>)> const & action_a) const override
	{
		parallel_traversal<vxldollar::uint256_t> (
		[&action_a, this] (vxldollar::uint256_t const & start, vxldollar::uint256_t const & end, bool const is_last) {
			auto transaction (this->store.tx_begin_read ());
			action_a (transaction, this->begin (transaction, start), !is_last ? this->begin (transaction, end) : this->end ());
		});
	}
};

}
