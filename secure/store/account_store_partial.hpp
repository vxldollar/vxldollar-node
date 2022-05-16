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
class account_store_partial : public account_store
{
private:
	vxldollar::store_partial<Val, Derived_Store> & store;

	friend void release_assert_success<Val, Derived_Store> (store_partial<Val, Derived_Store> const &, int const);

public:
	explicit account_store_partial (vxldollar::store_partial<Val, Derived_Store> & store_a) :
		store (store_a){};

	void put (vxldollar::write_transaction const & transaction_a, vxldollar::account const & account_a, vxldollar::account_info const & info_a) override
	{
		// Check we are still in sync with other tables
		vxldollar::db_val<Val> info (info_a);
		auto status = store.put (transaction_a, tables::accounts, account_a, info);
		release_assert_success (store, status);
	}

	bool get (vxldollar::transaction const & transaction_a, vxldollar::account const & account_a, vxldollar::account_info & info_a) override
	{
		vxldollar::db_val<Val> value;
		vxldollar::db_val<Val> account (account_a);
		auto status1 (store.get (transaction_a, tables::accounts, account, value));
		release_assert (store.success (status1) || store.not_found (status1));
		bool result (true);
		if (store.success (status1))
		{
			vxldollar::bufferstream stream (reinterpret_cast<uint8_t const *> (value.data ()), value.size ());
			result = info_a.deserialize (stream);
		}
		return result;
	}

	void del (vxldollar::write_transaction const & transaction_a, vxldollar::account const & account_a) override
	{
		auto status = store.del (transaction_a, tables::accounts, account_a);
		release_assert_success (store, status);
	}

	bool exists (vxldollar::transaction const & transaction_a, vxldollar::account const & account_a) override
	{
		auto iterator (begin (transaction_a, account_a));
		return iterator != end () && vxldollar::account (iterator->first) == account_a;
	}

	size_t count (vxldollar::transaction const & transaction_a) override
	{
		return store.count (transaction_a, tables::accounts);
	}

	vxldollar::store_iterator<vxldollar::account, vxldollar::account_info> begin (vxldollar::transaction const & transaction_a, vxldollar::account const & account_a) const override
	{
		return store.template make_iterator<vxldollar::account, vxldollar::account_info> (transaction_a, tables::accounts, vxldollar::db_val<Val> (account_a));
	}

	vxldollar::store_iterator<vxldollar::account, vxldollar::account_info> begin (vxldollar::transaction const & transaction_a) const override
	{
		return store.template make_iterator<vxldollar::account, vxldollar::account_info> (transaction_a, tables::accounts);
	}

	vxldollar::store_iterator<vxldollar::account, vxldollar::account_info> rbegin (vxldollar::transaction const & transaction_a) const override
	{
		return store.template make_iterator<vxldollar::account, vxldollar::account_info> (transaction_a, tables::accounts, false);
	}

	vxldollar::store_iterator<vxldollar::account, vxldollar::account_info> end () const override
	{
		return vxldollar::store_iterator<vxldollar::account, vxldollar::account_info> (nullptr);
	}

	void for_each_par (std::function<void (vxldollar::read_transaction const &, vxldollar::store_iterator<vxldollar::account, vxldollar::account_info>, vxldollar::store_iterator<vxldollar::account, vxldollar::account_info>)> const & action_a) const override
	{
		parallel_traversal<vxldollar::uint256_t> (
		[&action_a, this] (vxldollar::uint256_t const & start, vxldollar::uint256_t const & end, bool const is_last) {
			auto transaction (this->store.tx_begin_read ());
			action_a (transaction, this->begin (transaction, start), !is_last ? this->begin (transaction, end) : this->end ());
		});
	}
};

}
