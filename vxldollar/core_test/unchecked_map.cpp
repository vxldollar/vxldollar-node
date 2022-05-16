#include <vxldollar/lib/blockbuilders.hpp>
#include <vxldollar/lib/logger_mt.hpp>
#include <vxldollar/node/unchecked_map.hpp>
#include <vxldollar/secure/store.hpp>
#include <vxldollar/secure/utility.hpp>
#include <vxldollar/test_common/system.hpp>
#include <vxldollar/test_common/testutil.hpp>

#include <gtest/gtest.h>

#include <memory>

using namespace std::chrono_literals;

namespace
{
class context
{
public:
	context () :
		store{ vxldollar::make_store (logger, vxldollar::unique_path (), vxldollar::dev::constants) },
		unchecked{ *store, false }
	{
	}
	vxldollar::logger_mt logger;
	std::unique_ptr<vxldollar::store> store;
	vxldollar::unchecked_map unchecked;
};
std::shared_ptr<vxldollar::block> block ()
{
	vxldollar::block_builder builder;
	return builder.state ()
	.account (vxldollar::dev::genesis_key.pub)
	.previous (vxldollar::dev::genesis->hash ())
	.representative (vxldollar::dev::genesis_key.pub)
	.balance (vxldollar::dev::constants.genesis_amount - 1)
	.link (vxldollar::dev::genesis_key.pub)
	.sign (vxldollar::dev::genesis_key.prv, vxldollar::dev::genesis_key.pub)
	.work (0)
	.build_shared ();
}
}

TEST (unchecked_map, construction)
{
	context context;
}

TEST (unchecked_map, put_one)
{
	context context;
	vxldollar::unchecked_info info{ block (), vxldollar::dev::genesis_key.pub };
	context.unchecked.put (info.block->previous (), info);
}

TEST (block_store, one_bootstrap)
{
	vxldollar::system system{};
	vxldollar::logger_mt logger{};
	auto store = vxldollar::make_store (logger, vxldollar::unique_path (), vxldollar::dev::constants);
	vxldollar::unchecked_map unchecked{ *store, false };
	ASSERT_TRUE (!store->init_error ());
	auto block1 = std::make_shared<vxldollar::send_block> (0, 1, 2, vxldollar::keypair ().prv, 4, 5);
	unchecked.put (block1->hash (), vxldollar::unchecked_info{ block1 });
	auto check_block_is_listed = [&] (vxldollar::transaction const & transaction_a, vxldollar::block_hash const & block_hash_a) {
		return unchecked.get (transaction_a, block_hash_a).size () > 0;
	};
	// Waits for the block1 to get saved in the database
	ASSERT_TIMELY (10s, check_block_is_listed (store->tx_begin_read (), block1->hash ()));
	auto transaction = store->tx_begin_read ();
	auto [begin, end] = unchecked.full_range (transaction);
	ASSERT_NE (end, begin);
	auto hash1 = begin->first.key ();
	ASSERT_EQ (block1->hash (), hash1);
	auto blocks = unchecked.get (transaction, hash1);
	ASSERT_EQ (1, blocks.size ());
	auto block2 = blocks[0].block;
	ASSERT_EQ (*block1, *block2);
	++begin;
	ASSERT_EQ (end, begin);
}
