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
class peer_store_partial : public peer_store
{
private:
	vxldollar::store_partial<Val, Derived_Store> & store;

	friend void release_assert_success<Val, Derived_Store> (store_partial<Val, Derived_Store> const &, int const);

public:
	explicit peer_store_partial (vxldollar::store_partial<Val, Derived_Store> & store_a) :
		store (store_a){};

	void put (vxldollar::write_transaction const & transaction_a, vxldollar::endpoint_key const & endpoint_a) override
	{
		auto status = store.put_key (transaction_a, tables::peers, endpoint_a);
		release_assert_success (store, status);
	}

	void del (vxldollar::write_transaction const & transaction_a, vxldollar::endpoint_key const & endpoint_a) override
	{
		auto status (store.del (transaction_a, tables::peers, endpoint_a));
		release_assert_success (store, status);
	}

	bool exists (vxldollar::transaction const & transaction_a, vxldollar::endpoint_key const & endpoint_a) const override
	{
		return store.exists (transaction_a, tables::peers, vxldollar::db_val<Val> (endpoint_a));
	}

	size_t count (vxldollar::transaction const & transaction_a) const override
	{
		return store.count (transaction_a, tables::peers);
	}

	void clear (vxldollar::write_transaction const & transaction_a) override
	{
		auto status = store.drop (transaction_a, tables::peers);
		release_assert_success (store, status);
	}

	vxldollar::store_iterator<vxldollar::endpoint_key, vxldollar::no_value> begin (vxldollar::transaction const & transaction_a) const override
	{
		return store.template make_iterator<vxldollar::endpoint_key, vxldollar::no_value> (transaction_a, tables::peers);
	}

	vxldollar::store_iterator<vxldollar::endpoint_key, vxldollar::no_value> end () const override
	{
		return vxldollar::store_iterator<vxldollar::endpoint_key, vxldollar::no_value> (nullptr);
	}
};

}
