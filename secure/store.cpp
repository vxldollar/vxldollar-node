#include <vxldollar/lib/threading.hpp>
#include <vxldollar/secure/store.hpp>

vxldollar::representative_visitor::representative_visitor (vxldollar::transaction const & transaction_a, vxldollar::store & store_a) :
	transaction (transaction_a),
	store (store_a),
	result (0)
{
}

void vxldollar::representative_visitor::compute (vxldollar::block_hash const & hash_a)
{
	current = hash_a;
	while (result.is_zero ())
	{
		auto block (store.block.get (transaction, current));
		debug_assert (block != nullptr);
		block->visit (*this);
	}
}

void vxldollar::representative_visitor::send_block (vxldollar::send_block const & block_a)
{
	current = block_a.previous ();
}

void vxldollar::representative_visitor::receive_block (vxldollar::receive_block const & block_a)
{
	current = block_a.previous ();
}

void vxldollar::representative_visitor::open_block (vxldollar::open_block const & block_a)
{
	result = block_a.hash ();
}

void vxldollar::representative_visitor::change_block (vxldollar::change_block const & block_a)
{
	result = block_a.hash ();
}

void vxldollar::representative_visitor::state_block (vxldollar::state_block const & block_a)
{
	result = block_a.hash ();
}

vxldollar::read_transaction::read_transaction (std::unique_ptr<vxldollar::read_transaction_impl> read_transaction_impl) :
	impl (std::move (read_transaction_impl))
{
}

void * vxldollar::read_transaction::get_handle () const
{
	return impl->get_handle ();
}

void vxldollar::read_transaction::reset () const
{
	impl->reset ();
}

void vxldollar::read_transaction::renew () const
{
	impl->renew ();
}

void vxldollar::read_transaction::refresh () const
{
	reset ();
	renew ();
}

vxldollar::write_transaction::write_transaction (std::unique_ptr<vxldollar::write_transaction_impl> write_transaction_impl) :
	impl (std::move (write_transaction_impl))
{
	/*
	 * For IO threads, we do not want them to block on creating write transactions.
	 */
	debug_assert (vxldollar::thread_role::get () != vxldollar::thread_role::name::io);
}

void * vxldollar::write_transaction::get_handle () const
{
	return impl->get_handle ();
}

void vxldollar::write_transaction::commit ()
{
	impl->commit ();
}

void vxldollar::write_transaction::renew ()
{
	impl->renew ();
}

void vxldollar::write_transaction::refresh ()
{
	impl->commit ();
	impl->renew ();
}

bool vxldollar::write_transaction::contains (vxldollar::tables table_a) const
{
	return impl->contains (table_a);
}

// clang-format off
vxldollar::store::store (
	vxldollar::block_store & block_store_a,
	vxldollar::frontier_store & frontier_store_a,
	vxldollar::account_store & account_store_a,
	vxldollar::pending_store & pending_store_a,
	vxldollar::unchecked_store & unchecked_store_a,
	vxldollar::online_weight_store & online_weight_store_a,
	vxldollar::pruned_store & pruned_store_a,
	vxldollar::peer_store & peer_store_a,
	vxldollar::confirmation_height_store & confirmation_height_store_a,
	vxldollar::final_vote_store & final_vote_store_a,
	vxldollar::version_store & version_store_a
) :
	block (block_store_a),
	frontier (frontier_store_a),
	account (account_store_a),
	pending (pending_store_a),
	unchecked (unchecked_store_a),
	online_weight (online_weight_store_a),
	pruned (pruned_store_a),
	peer (peer_store_a),
	confirmation_height (confirmation_height_store_a),
	final_vote (final_vote_store_a),
	version (version_store_a)
{
}
// clang-format on

auto vxldollar::unchecked_store::equal_range (vxldollar::transaction const & transaction, vxldollar::block_hash const & dependency) -> std::pair<iterator, iterator>
{
	vxldollar::unchecked_key begin_l{ dependency, 0 };
	vxldollar::unchecked_key end_l{ vxldollar::block_hash{ dependency.number () + 1 }, 0 };
	// Adjust for edge case where number () + 1 wraps around.
	auto end_iter = begin_l.previous < end_l.previous ? lower_bound (transaction, end_l) : end ();
	return std::make_pair (lower_bound (transaction, begin_l), std::move (end_iter));
}

auto vxldollar::unchecked_store::full_range (vxldollar::transaction const & transaction) -> std::pair<iterator, iterator>
{
	return std::make_pair (begin (transaction), end ());
}

std::vector<vxldollar::unchecked_info> vxldollar::unchecked_store::get (vxldollar::transaction const & transaction, vxldollar::block_hash const & dependency)
{
	auto range = equal_range (transaction, dependency);
	std::vector<vxldollar::unchecked_info> result;
	auto & i = range.first;
	auto & n = range.second;
	for (; i != n; ++i)
	{
		auto const & key = i->first;
		auto const & value = i->second;
		debug_assert (key.hash == value.block->hash ());
		result.push_back (value);
	}
	return result;
}
