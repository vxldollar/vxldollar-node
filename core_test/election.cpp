#include <vxldollar/node/election.hpp>
#include <vxldollar/test_common/system.hpp>
#include <vxldollar/test_common/testutil.hpp>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

TEST (election, construction)
{
	vxldollar::system system (1);
	auto & node = *system.nodes[0];
	node.block_confirm (vxldollar::dev::genesis);
	node.scheduler.flush ();
	auto election = node.active.election (vxldollar::dev::genesis->qualified_root ());
	election->transition_active ();
}

TEST (election, quorum_minimum_flip_success)
{
	vxldollar::system system{};

	vxldollar::node_config node_config{ vxldollar::get_available_port (), system.logging };
	node_config.online_weight_minimum = vxldollar::dev::constants.genesis_amount;
	node_config.frontiers_confirmation = vxldollar::frontiers_confirmation_mode::disabled;

	auto & node1 = *system.add_node (node_config);
	auto const latest_hash = vxldollar::dev::genesis->hash ();
	vxldollar::state_block_builder builder{};

	vxldollar::keypair key1{};
	auto send1 = builder.make_block ()
				 .previous (latest_hash)
				 .account (vxldollar::dev::genesis_key.pub)
				 .representative (vxldollar::dev::genesis_key.pub)
				 .balance (node1.online_reps.delta ())
				 .link (key1.pub)
				 .work (*system.work.generate (latest_hash))
				 .sign (vxldollar::dev::genesis_key.prv, vxldollar::dev::genesis_key.pub)
				 .build_shared ();

	vxldollar::keypair key2{};
	auto send2 = builder.make_block ()
				 .previous (latest_hash)
				 .account (vxldollar::dev::genesis_key.pub)
				 .representative (vxldollar::dev::genesis_key.pub)
				 .balance (node1.online_reps.delta ())
				 .link (key2.pub)
				 .work (*system.work.generate (latest_hash))
				 .sign (vxldollar::dev::genesis_key.prv, vxldollar::dev::genesis_key.pub)
				 .build_shared ();

	node1.process_active (send1);
	ASSERT_TIMELY (5s, node1.active.election (send1->qualified_root ()) != nullptr)

	node1.process_active (send2);
	std::shared_ptr<vxldollar::election> election{};
	ASSERT_TIMELY (5s, (election = node1.active.election (send2->qualified_root ())) != nullptr)
	ASSERT_TIMELY (5s, election->blocks ().size () == 2);

	auto const vote1 = std::make_shared<vxldollar::vote> (vxldollar::dev::genesis_key.pub, vxldollar::dev::genesis_key.prv, vxldollar::vote::timestamp_max, vxldollar::vote::duration_max, send2);
	ASSERT_EQ (vxldollar::vote_code::vote, node1.active.vote (vote1));

	ASSERT_TIMELY (5s, election->confirmed ());
	auto const winner = election->winner ();
	ASSERT_NE (nullptr, winner);
	ASSERT_EQ (*winner, *send2);
}

TEST (election, quorum_minimum_flip_fail)
{
	vxldollar::system system{};

	vxldollar::node_config node_config{ vxldollar::get_available_port (), system.logging };
	node_config.online_weight_minimum = vxldollar::dev::constants.genesis_amount;
	node_config.frontiers_confirmation = vxldollar::frontiers_confirmation_mode::disabled;

	auto & node1 = *system.add_node (node_config);
	auto const latest_hash = vxldollar::dev::genesis->hash ();
	vxldollar::state_block_builder builder{};

	vxldollar::keypair key1{};
	auto send1 = builder.make_block ()
				 .previous (latest_hash)
				 .account (vxldollar::dev::genesis_key.pub)
				 .representative (vxldollar::dev::genesis_key.pub)
				 .balance (node1.online_reps.delta () - 1)
				 .link (key1.pub)
				 .work (*system.work.generate (latest_hash))
				 .sign (vxldollar::dev::genesis_key.prv, vxldollar::dev::genesis_key.pub)
				 .build_shared ();

	vxldollar::keypair key2{};
	auto send2 = builder.make_block ()
				 .previous (latest_hash)
				 .account (vxldollar::dev::genesis_key.pub)
				 .representative (vxldollar::dev::genesis_key.pub)
				 .balance (node1.online_reps.delta () - 1)
				 .link (key2.pub)
				 .work (*system.work.generate (latest_hash))
				 .sign (vxldollar::dev::genesis_key.prv, vxldollar::dev::genesis_key.pub)
				 .build_shared ();

	node1.process_active (send1);
	ASSERT_TIMELY (5s, node1.active.election (send1->qualified_root ()) != nullptr)

	node1.process_active (send2);
	std::shared_ptr<vxldollar::election> election{};
	ASSERT_TIMELY (5s, (election = node1.active.election (send2->qualified_root ())) != nullptr)
	ASSERT_TIMELY (5s, election->blocks ().size () == 2);

	auto const vote1 = std::make_shared<vxldollar::vote> (vxldollar::dev::genesis_key.pub, vxldollar::dev::genesis_key.prv, vxldollar::vote::timestamp_max, vxldollar::vote::duration_max, send2);
	ASSERT_EQ (vxldollar::vote_code::vote, node1.active.vote (vote1));

	// give the election 5 seconds before asserting it is not confirmed so that in case
	// it would be wrongfully confirmed, have that immediately fail instead of race
	//
	std::this_thread::sleep_for (5s);
	ASSERT_FALSE (election->confirmed ());
	ASSERT_FALSE (node1.block_confirmed (send2->hash ()));
}

TEST (election, quorum_minimum_confirm_success)
{
	vxldollar::system system;
	vxldollar::node_config node_config (vxldollar::get_available_port (), system.logging);
	node_config.online_weight_minimum = vxldollar::dev::constants.genesis_amount;
	node_config.frontiers_confirmation = vxldollar::frontiers_confirmation_mode::disabled;
	auto & node1 = *system.add_node (node_config);
	vxldollar::keypair key1;
	vxldollar::block_builder builder;
	auto send1 = builder.state ()
				 .account (vxldollar::dev::genesis_key.pub)
				 .previous (vxldollar::dev::genesis->hash ())
				 .representative (vxldollar::dev::genesis_key.pub)
				 .balance (node1.online_reps.delta ())
				 .link (key1.pub)
				 .work (0)
				 .sign (vxldollar::dev::genesis_key.prv, vxldollar::dev::genesis_key.pub)
				 .build_shared ();
	node1.work_generate_blocking (*send1);
	node1.process_active (send1);
	node1.block_processor.flush ();
	node1.scheduler.activate (vxldollar::dev::genesis_key.pub, node1.store.tx_begin_read ());
	node1.scheduler.flush ();
	auto election = node1.active.election (send1->qualified_root ());
	ASSERT_NE (nullptr, election);
	ASSERT_EQ (1, election->blocks ().size ());
	auto vote1 (std::make_shared<vxldollar::vote> (vxldollar::dev::genesis_key.pub, vxldollar::dev::genesis_key.prv, vxldollar::vote::timestamp_max, vxldollar::vote::duration_max, send1));
	ASSERT_EQ (vxldollar::vote_code::vote, node1.active.vote (vote1));
	node1.block_processor.flush ();
	ASSERT_NE (nullptr, node1.block (send1->hash ()));
	ASSERT_TRUE (election->confirmed ());
}

TEST (election, quorum_minimum_confirm_fail)
{
	vxldollar::system system;
	vxldollar::node_config node_config (vxldollar::get_available_port (), system.logging);
	node_config.online_weight_minimum = vxldollar::dev::constants.genesis_amount;
	node_config.frontiers_confirmation = vxldollar::frontiers_confirmation_mode::disabled;
	auto & node1 = *system.add_node (node_config);
	vxldollar::keypair key1;
	vxldollar::block_builder builder;
	auto send1 = builder.state ()
				 .account (vxldollar::dev::genesis_key.pub)
				 .previous (vxldollar::dev::genesis->hash ())
				 .representative (vxldollar::dev::genesis_key.pub)
				 .balance (node1.online_reps.delta () - 1)
				 .link (key1.pub)
				 .work (0)
				 .sign (vxldollar::dev::genesis_key.prv, vxldollar::dev::genesis_key.pub)
				 .build_shared ();
	node1.work_generate_blocking (*send1);
	node1.process_active (send1);
	node1.block_processor.flush ();
	node1.scheduler.activate (vxldollar::dev::genesis_key.pub, node1.store.tx_begin_read ());
	node1.scheduler.flush ();
	auto election = node1.active.election (send1->qualified_root ());
	ASSERT_NE (nullptr, election);
	ASSERT_EQ (1, election->blocks ().size ());
	auto vote1 (std::make_shared<vxldollar::vote> (vxldollar::dev::genesis_key.pub, vxldollar::dev::genesis_key.prv, vxldollar::vote::timestamp_max, vxldollar::vote::duration_max, send1));
	ASSERT_EQ (vxldollar::vote_code::vote, node1.active.vote (vote1));
	node1.block_processor.flush ();
	ASSERT_NE (nullptr, node1.block (send1->hash ()));
	ASSERT_FALSE (election->confirmed ());
}

namespace vxldollar
{
TEST (election, quorum_minimum_update_weight_before_quorum_checks)
{
	vxldollar::system system{};

	vxldollar::node_config node_config{ vxldollar::get_available_port (), system.logging };
	node_config.frontiers_confirmation = vxldollar::frontiers_confirmation_mode::disabled;

	auto & node1 = *system.add_node (node_config);
	system.wallet (0)->insert_adhoc (vxldollar::dev::genesis_key.prv);

	vxldollar::keypair key1{};
	vxldollar::send_block_builder builder{};
	auto const amount = ((vxldollar::uint256_t (node_config.online_weight_minimum.number ()) * vxldollar::online_reps::online_weight_quorum) / 100).convert_to<vxldollar::uint128_t> () - 1;

	auto const latest = node1.latest (vxldollar::dev::genesis_key.pub);
	auto const send1 = builder.make_block ()
					   .previous (latest)
					   .destination (key1.pub)
					   .balance (amount)
					   .sign (vxldollar::dev::genesis_key.prv, vxldollar::dev::genesis_key.pub)
					   .work (*system.work.generate (latest))
					   .build_shared ();
	node1.process_active (send1);

	auto const open1 = vxldollar::open_block_builder{}.make_block ().account (key1.pub).source (send1->hash ()).representative (key1.pub).sign (key1.prv, key1.pub).work (*system.work.generate (key1.pub)).build_shared ();
	ASSERT_EQ (vxldollar::process_result::progress, node1.process (*open1).code);

	vxldollar::keypair key2{};
	auto const send2 = builder.make_block ()
					   .previous (open1->hash ())
					   .destination (key2.pub)
					   .balance (3)
					   .sign (key1.prv, key1.pub)
					   .work (*system.work.generate (open1->hash ()))
					   .build_shared ();
	ASSERT_EQ (vxldollar::process_result::progress, node1.process (*send2).code);
	ASSERT_TIMELY (5s, node1.ledger.cache.block_count == 4);

	node_config.peering_port = vxldollar::get_available_port ();
	auto & node2 = *system.add_node (node_config);

	system.wallet (1)->insert_adhoc (key1.prv);
	ASSERT_TIMELY (10s, node2.ledger.cache.block_count == 4);

	std::shared_ptr<vxldollar::election> election{};
	ASSERT_TIMELY (5s, (election = node1.active.election (send1->qualified_root ())) != nullptr);
	ASSERT_EQ (1, election->blocks ().size ());

	auto const vote1 = std::make_shared<vxldollar::vote> (vxldollar::dev::genesis_key.pub, vxldollar::dev::genesis_key.prv, vxldollar::vote::timestamp_max, vxldollar::vote::duration_max, send1);
	ASSERT_EQ (vxldollar::vote_code::vote, node1.active.vote (vote1));

	auto channel = node1.network.find_channel (node2.network.endpoint ());
	ASSERT_NE (channel, nullptr);

	auto const vote2 = std::make_shared<vxldollar::vote> (key1.pub, key1.prv, vxldollar::vote::timestamp_max, vxldollar::vote::duration_max, send1);
	ASSERT_TIMELY (10s, !node1.rep_crawler.response (channel, vote2));

	ASSERT_FALSE (election->confirmed ());
	{
		vxldollar::lock_guard<vxldollar::mutex> guard (node1.online_reps.mutex);
		// Modify online_m for online_reps to more than is available, this checks that voting below updates it to current online reps.
		node1.online_reps.online_m = node_config.online_weight_minimum.number () + 20;
	}
	ASSERT_EQ (vxldollar::vote_code::vote, node1.active.vote (vote2));
	ASSERT_TRUE (election->confirmed ());
	ASSERT_NE (nullptr, node1.block (send1->hash ()));
}
}
