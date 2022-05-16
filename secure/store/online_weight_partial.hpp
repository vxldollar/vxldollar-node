#pragma once

#include <vxldollar/secure/store_partial.hpp>

namespace vxldollar
{
template <typename Val, typename Derived_Store>
class store_partial;

template <typename Val, typename Derived_Store>
void release_assert_success (store_partial<Val, Derived_Store> const &, int const);

template <typename Val, typename Derived_Store>
class online_weight_store_partial : public online_weight_store
{
private:
	vxldollar::store_partial<Val, Derived_Store> & store;

	friend void release_assert_success<Val, Derived_Store> (store_partial<Val, Derived_Store> const &, int const);

public:
	explicit online_weight_store_partial (vxldollar::store_partial<Val, Derived_Store> & store_a) :
		store (store_a){};

	void put (vxldollar::write_transaction const & transaction_a, uint64_t time_a, vxldollar::amount const & amount_a) override
	{
		vxldollar::db_val<Val> value (amount_a);
		auto status (store.put (transaction_a, tables::online_weight, time_a, value));
		release_assert_success (store, status);
	}

	void del (vxldollar::write_transaction const & transaction_a, uint64_t time_a) override
	{
		auto status (store.del (transaction_a, tables::online_weight, time_a));
		release_assert_success (store, status);
	}

	vxldollar::store_iterator<uint64_t, vxldollar::amount> begin (vxldollar::transaction const & transaction_a) const override
	{
		return store.template make_iterator<uint64_t, vxldollar::amount> (transaction_a, tables::online_weight);
	}

	vxldollar::store_iterator<uint64_t, vxldollar::amount> rbegin (vxldollar::transaction const & transaction_a) const override
	{
		return store.template make_iterator<uint64_t, vxldollar::amount> (transaction_a, tables::online_weight, false);
	}

	vxldollar::store_iterator<uint64_t, vxldollar::amount> end () const override
	{
		return vxldollar::store_iterator<uint64_t, vxldollar::amount> (nullptr);
	}

	size_t count (vxldollar::transaction const & transaction_a) const override
	{
		return store.count (transaction_a, tables::online_weight);
	}

	void clear (vxldollar::write_transaction const & transaction_a) override
	{
		auto status (store.drop (transaction_a, tables::online_weight));
		release_assert_success (store, status);
	}
};

}
