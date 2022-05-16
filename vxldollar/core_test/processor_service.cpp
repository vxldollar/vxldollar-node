#include <vxldollar/lib/stats.hpp>
#include <vxldollar/lib/work.hpp>
#include <vxldollar/secure/ledger.hpp>
#include <vxldollar/secure/store.hpp>
#include <vxldollar/secure/utility.hpp>
#include <vxldollar/test_common/testutil.hpp>

#include <gtest/gtest.h>

TEST (processor_service, bad_send_signature)
{
	vxldollar::logger_mt logger;
	auto store = vxldollar::make_store (logger, vxldollar::unique_path (), vxldollar::dev::constants);
	ASSERT_FALSE (store->init_error ());
	vxldollar::stat stats;
	vxldollar::ledger ledger (*store, stats, vxldollar::dev::constants);
	auto transaction (store->tx_begin_write ());
	store->initialize (transaction, ledger.cache);
	vxldollar::work_pool pool{ vxldollar::dev::network_params.network, std::numeric_limits<unsigned>::max () };
	vxldollar::account_info info1;
	ASSERT_FALSE (store->account.get (transaction, vxldollar::dev::genesis_key.pub, info1));
	vxldollar::keypair key2;
	vxldollar::send_block send (info1.head, vxldollar::dev::genesis_key.pub, 50, vxldollar::dev::genesis_key.prv, vxldollar::dev::genesis_key.pub, *pool.generate (info1.head));
	send.signature.bytes[32] ^= 0x1;
	ASSERT_EQ (vxldollar::process_result::bad_signature, ledger.process (transaction, send).code);
}

TEST (processor_service, bad_receive_signature)
{
	vxldollar::logger_mt logger;
	auto store = vxldollar::make_store (logger, vxldollar::unique_path (), vxldollar::dev::constants);
	ASSERT_FALSE (store->init_error ());
	vxldollar::stat stats;
	vxldollar::ledger ledger (*store, stats, vxldollar::dev::constants);
	auto transaction (store->tx_begin_write ());
	store->initialize (transaction, ledger.cache);
	vxldollar::work_pool pool{ vxldollar::dev::network_params.network, std::numeric_limits<unsigned>::max () };
	vxldollar::account_info info1;
	ASSERT_FALSE (store->account.get (transaction, vxldollar::dev::genesis_key.pub, info1));
	vxldollar::send_block send (info1.head, vxldollar::dev::genesis_key.pub, 50, vxldollar::dev::genesis_key.prv, vxldollar::dev::genesis_key.pub, *pool.generate (info1.head));
	vxldollar::block_hash hash1 (send.hash ());
	ASSERT_EQ (vxldollar::process_result::progress, ledger.process (transaction, send).code);
	vxldollar::account_info info2;
	ASSERT_FALSE (store->account.get (transaction, vxldollar::dev::genesis_key.pub, info2));
	vxldollar::receive_block receive (hash1, hash1, vxldollar::dev::genesis_key.prv, vxldollar::dev::genesis_key.pub, *pool.generate (hash1));
	receive.signature.bytes[32] ^= 0x1;
	ASSERT_EQ (vxldollar::process_result::bad_signature, ledger.process (transaction, receive).code);
}
