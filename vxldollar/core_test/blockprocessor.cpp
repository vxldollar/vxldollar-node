#include <vxldollar/lib/blockbuilders.hpp>
#include <vxldollar/node/node.hpp>
#include <vxldollar/node/nodeconfig.hpp>
#include <vxldollar/secure/common.hpp>
#include <vxldollar/secure/ledger.hpp>
#include <vxldollar/test_common/system.hpp>
#include <vxldollar/test_common/testutil.hpp>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

TEST (block_processor, broadcast_block_on_arrival)
{
	vxldollar::system system;
	vxldollar::node_config config1{ vxldollar::get_available_port (), system.logging };
	// Deactivates elections on both nodes.
	config1.active_elections_size = 0;
	vxldollar::node_config config2{ vxldollar::get_available_port (), system.logging };
	config2.active_elections_size = 0;
	vxldollar::node_flags flags;
	// Disables bootstrap listener to make sure the block won't be shared by this channel.
	flags.disable_bootstrap_listener = true;
	auto node1 = system.add_node (config1, flags);
	auto node2 = system.add_node (config2, flags);
	vxldollar::state_block_builder builder;
	auto send1 = builder.make_block ()
				 .account (vxldollar::dev::genesis_key.pub)
				 .previous (vxldollar::dev::genesis->hash ())
				 .representative (vxldollar::dev::genesis_key.pub)
				 .balance (vxldollar::dev::constants.genesis_amount - vxldollar::Gxrb_ratio)
				 .link (vxldollar::dev::genesis_key.pub)
				 .sign (vxldollar::dev::genesis_key.prv, vxldollar::dev::genesis_key.pub)
				 .work (*system.work.generate (vxldollar::dev::genesis->hash ()))
				 .build_shared ();
	// Adds a block to the first node. process_active() -> (calls) block_processor.add() -> add() ->
	// awakes process_block() -> process_batch() -> process_one() -> process_live()
	node1->process_active (send1);
	// Checks whether the block was broadcast.
	ASSERT_TIMELY (5s, node2->ledger.block_or_pruned_exists (send1->hash ()));
}