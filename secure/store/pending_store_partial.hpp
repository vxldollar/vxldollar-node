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
class pending_store_partial : public pending_store
{
private:
	vxldollar::store_partial<Val, Derived_Store> & store;

	friend void release_assert_success<Val, Derived_Store> (store_partial<Val, Derived_Store> const &, int const);

public:
	explicit pending_store_partial (vxldollar::store_partial<Val, Derived_Store> & store_a) :
		store (store_a){};

	void put (vxldollar::write_transaction const & transaction_a, vxldollar::pending_key const & key_a, vxldollar::pending_info const & pending_info_a) override
	{
		vxldollar::db_val<Val> pending (pending_info_a);
		auto status = store.put (transaction_a, tables::pending, key_a, pending);
		release_assert_success (store, status);
	}

	void del (vxldollar::write_transaction const & transaction_a, vxldollar::pending_key const & key_a) override
	{
		auto status = store.del (transaction_a, tables::pending, key_a);
		release_assert_success (store, status);
	}

	bool get (vxldollar::transaction const & transaction_a, vxldollar::pending_key const & key_a, vxldollar::pending_info & pending_a) override
	{
		vxldollar::db_val<Val> value;
		vxldollar::db_val<Val> key (key_a);
		auto status1 = store.get (transaction_a, tables::pending, key, value);
		release_assert (store.success (status1) || store.not_found (status1));
		bool result (true);
		if (store.success (status1))
		{
			vxldollar::bufferstream stream (reinterpret_cast<uint8_t const *> (value.data ()), value.size ());
			result = pending_a.deserialize (stream);
		}
		return result;
	}

	bool exists (vxldollar::transaction const & transaction_a, vxldollar::pending_key const & key_a) override
	{
		auto iterator (begin (transaction_a, key_a));
		return iterator != end () && vxldollar::pending_key (iterator->first) == key_a;
	}

	bool any (vxldollar::transaction const & transaction_a, vxldollar::account const & account_a) override
	{
		auto iterator (begin (transaction_a, vxldollar::pending_key (account_a, 0)));
		return iterator != end () && vxldollar::pending_key (iterator->first).account == account_a;
	}

	vxldollar::store_iterator<vxldollar::pending_key, vxldollar::pending_info> begin (vxldollar::transaction const & transaction_a, vxldollar::pending_key const & key_a) const override
	{
		return store.template make_iterator<vxldollar::pending_key, vxldollar::pending_info> (transaction_a, tables::pending, vxldollar::db_val<Val> (key_a));
	}

	vxldollar::store_iterator<vxldollar::pending_key, vxldollar::pending_info> begin (vxldollar::transaction const & transaction_a) const override
	{
		return store.template make_iterator<vxldollar::pending_key, vxldollar::pending_info> (transaction_a, tables::pending);
	}

	vxldollar::store_iterator<vxldollar::pending_key, vxldollar::pending_info> end () const override
	{
		return vxldollar::store_iterator<vxldollar::pending_key, vxldollar::pending_info> (nullptr);
	}

	void for_each_par (std::function<void (vxldollar::read_transaction const &, vxldollar::store_iterator<vxldollar::pending_key, vxldollar::pending_info>, vxldollar::store_iterator<vxldollar::pending_key, vxldollar::pending_info>)> const & action_a) const override
	{
		parallel_traversal<vxldollar::uint512_t> (
		[&action_a, this] (vxldollar::uint512_t const & start, vxldollar::uint512_t const & end, bool const is_last) {
			vxldollar::uint512_union union_start (start);
			vxldollar::uint512_union union_end (end);
			vxldollar::pending_key key_start (union_start.uint256s[0].number (), union_start.uint256s[1].number ());
			vxldollar::pending_key key_end (union_end.uint256s[0].number (), union_end.uint256s[1].number ());
			auto transaction (this->store.tx_begin_read ());
			action_a (transaction, this->begin (transaction, key_start), !is_last ? this->begin (transaction, key_end) : this->end ());
		});
	}
};

}
