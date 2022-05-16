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
class final_vote_store_partial : public final_vote_store
{
private:
	vxldollar::store_partial<Val, Derived_Store> & store;

	friend void release_assert_success<Val, Derived_Store> (store_partial<Val, Derived_Store> const &, int const);

public:
	explicit final_vote_store_partial (vxldollar::store_partial<Val, Derived_Store> & store_a) :
		store (store_a){};

	bool put (vxldollar::write_transaction const & transaction_a, vxldollar::qualified_root const & root_a, vxldollar::block_hash const & hash_a) override
	{
		vxldollar::db_val<Val> value;
		auto status = store.get (transaction_a, tables::final_votes, vxldollar::db_val<Val> (root_a), value);
		release_assert (store.success (status) || store.not_found (status));
		bool result (true);
		if (store.success (status))
		{
			result = static_cast<vxldollar::block_hash> (value) == hash_a;
		}
		else
		{
			status = store.put (transaction_a, tables::final_votes, root_a, hash_a);
			release_assert_success (store, status);
		}
		return result;
	}

	std::vector<vxldollar::block_hash> get (vxldollar::transaction const & transaction_a, vxldollar::root const & root_a) override
	{
		std::vector<vxldollar::block_hash> result;
		vxldollar::qualified_root key_start (root_a.raw, 0);
		for (auto i (begin (transaction_a, key_start)), n (end ()); i != n && vxldollar::qualified_root (i->first).root () == root_a; ++i)
		{
			result.push_back (i->second);
		}
		return result;
	}

	void del (vxldollar::write_transaction const & transaction_a, vxldollar::root const & root_a) override
	{
		std::vector<vxldollar::qualified_root> final_vote_qualified_roots;
		for (auto i (begin (transaction_a, vxldollar::qualified_root (root_a.raw, 0))), n (end ()); i != n && vxldollar::qualified_root (i->first).root () == root_a; ++i)
		{
			final_vote_qualified_roots.push_back (i->first);
		}

		for (auto & final_vote_qualified_root : final_vote_qualified_roots)
		{
			auto status (store.del (transaction_a, tables::final_votes, vxldollar::db_val<Val> (final_vote_qualified_root)));
			release_assert_success (store, status);
		}
	}

	size_t count (vxldollar::transaction const & transaction_a) const override
	{
		return store.count (transaction_a, tables::final_votes);
	}

	void clear (vxldollar::write_transaction const & transaction_a, vxldollar::root const & root_a) override
	{
		del (transaction_a, root_a);
	}

	void clear (vxldollar::write_transaction const & transaction_a) override
	{
		store.drop (transaction_a, vxldollar::tables::final_votes);
	}

	vxldollar::store_iterator<vxldollar::qualified_root, vxldollar::block_hash> begin (vxldollar::transaction const & transaction_a, vxldollar::qualified_root const & root_a) const override
	{
		return store.template make_iterator<vxldollar::qualified_root, vxldollar::block_hash> (transaction_a, tables::final_votes, vxldollar::db_val<Val> (root_a));
	}

	vxldollar::store_iterator<vxldollar::qualified_root, vxldollar::block_hash> begin (vxldollar::transaction const & transaction_a) const override
	{
		return store.template make_iterator<vxldollar::qualified_root, vxldollar::block_hash> (transaction_a, tables::final_votes);
	}

	vxldollar::store_iterator<vxldollar::qualified_root, vxldollar::block_hash> end () const override
	{
		return vxldollar::store_iterator<vxldollar::qualified_root, vxldollar::block_hash> (nullptr);
	}

	void for_each_par (std::function<void (vxldollar::read_transaction const &, vxldollar::store_iterator<vxldollar::qualified_root, vxldollar::block_hash>, vxldollar::store_iterator<vxldollar::qualified_root, vxldollar::block_hash>)> const & action_a) const override
	{
		parallel_traversal<vxldollar::uint512_t> (
		[&action_a, this] (vxldollar::uint512_t const & start, vxldollar::uint512_t const & end, bool const is_last) {
			auto transaction (this->store.tx_begin_read ());
			action_a (transaction, this->begin (transaction, start), !is_last ? this->begin (transaction, end) : this->end ());
		});
	}
};

}
