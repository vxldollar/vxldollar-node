#include <vxldollar/lib/jsonconfig.hpp>
#include <vxldollar/node/vote_processor.hpp>
#include <vxldollar/test_common/system.hpp>
#include <vxldollar/test_common/testutil.hpp>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

TEST (vote_processor, codes)
{
	vxldollar::system system (1);
	auto & node (*system.nodes[0]);
	vxldollar::keypair key;
	auto vote (std::make_shared<vxldollar::vote> (key.pub, key.prv, vxldollar::vote::timestamp_min * 1, 0, std::vector<vxldollar::block_hash>{ vxldollar::dev::genesis->hash () }));
	auto vote_invalid = std::make_shared<vxldollar::vote> (*vote);
	vote_invalid->signature.bytes[0] ^= 1;
	auto channel (std::make_shared<vxldollar::transport::channel_loopback> (node));

	// Invalid signature
	ASSERT_EQ (vxldollar::vote_code::invalid, node.vote_processor.vote_blocking (vote_invalid, channel, false));

	// Hint of pre-validation
	ASSERT_NE (vxldollar::vote_code::invalid, node.vote_processor.vote_blocking (vote_invalid, channel, true));

	// No ongoing election
	ASSERT_EQ (vxldollar::vote_code::indeterminate, node.vote_processor.vote_blocking (vote, channel));

	// First vote from an account for an ongoing election
	node.block_confirm (vxldollar::dev::genesis);
	ASSERT_NE (nullptr, node.active.election (vxldollar::dev::genesis->qualified_root ()));
	ASSERT_EQ (vxldollar::vote_code::vote, node.vote_processor.vote_blocking (vote, channel));

	// Processing the same vote is a replay
	ASSERT_EQ (vxldollar::vote_code::replay, node.vote_processor.vote_blocking (vote, channel));

	// Invalid takes precedence
	ASSERT_EQ (vxldollar::vote_code::invalid, node.vote_processor.vote_blocking (vote_invalid, channel));

	// Once the election is removed (confirmed / dropped) the vote is again indeterminate
	node.active.erase (*vxldollar::dev::genesis);
	ASSERT_EQ (vxldollar::vote_code::indeterminate, node.vote_processor.vote_blocking (vote, channel));
}

TEST (vote_processor, flush)
{
	vxldollar::system system (1);
	auto & node (*system.nodes[0]);
	auto channel (std::make_shared<vxldollar::transport::channel_loopback> (node));
	for (unsigned i = 0; i < 2000; ++i)
	{
		auto vote = std::make_shared<vxldollar::vote> (vxldollar::dev::genesis_key.pub, vxldollar::dev::genesis_key.prv, vxldollar::vote::timestamp_min * (1 + i), 0, std::vector<vxldollar::block_hash>{ vxldollar::dev::genesis->hash () });
		node.vote_processor.vote (vote, channel);
	}
	node.vote_processor.flush ();
	ASSERT_TRUE (node.vote_processor.empty ());
}

TEST (vote_processor, invalid_signature)
{
	vxldollar::system system{ 1 };
	auto & node = *system.nodes[0];
	vxldollar::keypair key;
	auto vote = std::make_shared<vxldollar::vote> (key.pub, key.prv, vxldollar::vote::timestamp_min * 1, 0, std::vector<vxldollar::block_hash>{ vxldollar::dev::genesis->hash () });
	auto vote_invalid = std::make_shared<vxldollar::vote> (*vote);
	vote_invalid->signature.bytes[0] ^= 1;
	auto channel = std::make_shared<vxldollar::transport::channel_loopback> (node);

	node.block_confirm (vxldollar::dev::genesis);
	auto election = node.active.election (vxldollar::dev::genesis->qualified_root ());
	ASSERT_TRUE (election);
	ASSERT_EQ (1, election->votes ().size ());
	node.vote_processor.vote (vote_invalid, channel);
	node.vote_processor.flush ();
	ASSERT_EQ (1, election->votes ().size ());
	node.vote_processor.vote (vote, channel);
	node.vote_processor.flush ();
	ASSERT_EQ (2, election->votes ().size ());
}

TEST (vote_processor, no_capacity)
{
	vxldollar::system system;
	vxldollar::node_flags node_flags;
	node_flags.vote_processor_capacity = 0;
	auto & node (*system.add_node (node_flags));
	vxldollar::keypair key;
	auto vote (std::make_shared<vxldollar::vote> (key.pub, key.prv, vxldollar::vote::timestamp_min * 1, 0, std::vector<vxldollar::block_hash>{ vxldollar::dev::genesis->hash () }));
	auto channel (std::make_shared<vxldollar::transport::channel_loopback> (node));
	ASSERT_TRUE (node.vote_processor.vote (vote, channel));
}

TEST (vote_processor, overflow)
{
	vxldollar::system system;
	vxldollar::node_flags node_flags;
	node_flags.vote_processor_capacity = 1;
	auto & node (*system.add_node (node_flags));
	vxldollar::keypair key;
	auto vote (std::make_shared<vxldollar::vote> (key.pub, key.prv, vxldollar::vote::timestamp_min * 1, 0, std::vector<vxldollar::block_hash>{ vxldollar::dev::genesis->hash () }));
	auto channel (std::make_shared<vxldollar::transport::channel_loopback> (node));

	// No way to lock the processor, but queueing votes in quick succession must result in overflow
	size_t not_processed{ 0 };
	size_t const total{ 1000 };
	for (unsigned i = 0; i < total; ++i)
	{
		if (node.vote_processor.vote (vote, channel))
		{
			++not_processed;
		}
	}
	ASSERT_GT (not_processed, 0);
	ASSERT_LT (not_processed, total);
	ASSERT_EQ (not_processed, node.stats.count (vxldollar::stat::type::vote, vxldollar::stat::detail::vote_overflow));
}

namespace vxldollar
{
TEST (vote_processor, weights)
{
	vxldollar::system system (4);
	auto & node (*system.nodes[0]);

	// Create representatives of different weight levels
	// The online stake will be the minimum configurable due to online_reps sampling in tests
	auto const online = node.config.online_weight_minimum.number ();
	auto const level0 = online / 5000; // 0.02%
	auto const level1 = online / 500; // 0.2%
	auto const level2 = online / 50; // 2%

	vxldollar::keypair key0;
	vxldollar::keypair key1;
	vxldollar::keypair key2;

	system.wallet (0)->insert_adhoc (vxldollar::dev::genesis_key.prv);
	system.wallet (1)->insert_adhoc (key0.prv);
	system.wallet (2)->insert_adhoc (key1.prv);
	system.wallet (3)->insert_adhoc (key2.prv);
	system.wallet (1)->store.representative_set (system.nodes[1]->wallets.tx_begin_write (), key0.pub);
	system.wallet (2)->store.representative_set (system.nodes[2]->wallets.tx_begin_write (), key1.pub);
	system.wallet (3)->store.representative_set (system.nodes[3]->wallets.tx_begin_write (), key2.pub);
	system.wallet (0)->send_sync (vxldollar::dev::genesis_key.pub, key0.pub, level0);
	system.wallet (0)->send_sync (vxldollar::dev::genesis_key.pub, key1.pub, level1);
	system.wallet (0)->send_sync (vxldollar::dev::genesis_key.pub, key2.pub, level2);

	// Wait for representatives
	ASSERT_TIMELY (10s, node.ledger.cache.rep_weights.get_rep_amounts ().size () == 4);
	node.vote_processor.calculate_weights ();

	ASSERT_EQ (node.vote_processor.representatives_1.end (), node.vote_processor.representatives_1.find (key0.pub));
	ASSERT_EQ (node.vote_processor.representatives_2.end (), node.vote_processor.representatives_2.find (key0.pub));
	ASSERT_EQ (node.vote_processor.representatives_3.end (), node.vote_processor.representatives_3.find (key0.pub));

	ASSERT_NE (node.vote_processor.representatives_1.end (), node.vote_processor.representatives_1.find (key1.pub));
	ASSERT_EQ (node.vote_processor.representatives_2.end (), node.vote_processor.representatives_2.find (key1.pub));
	ASSERT_EQ (node.vote_processor.representatives_3.end (), node.vote_processor.representatives_3.find (key1.pub));

	ASSERT_NE (node.vote_processor.representatives_1.end (), node.vote_processor.representatives_1.find (key2.pub));
	ASSERT_NE (node.vote_processor.representatives_2.end (), node.vote_processor.representatives_2.find (key2.pub));
	ASSERT_EQ (node.vote_processor.representatives_3.end (), node.vote_processor.representatives_3.find (key2.pub));

	ASSERT_NE (node.vote_processor.representatives_1.end (), node.vote_processor.representatives_1.find (vxldollar::dev::genesis_key.pub));
	ASSERT_NE (node.vote_processor.representatives_2.end (), node.vote_processor.representatives_2.find (vxldollar::dev::genesis_key.pub));
	ASSERT_NE (node.vote_processor.representatives_3.end (), node.vote_processor.representatives_3.find (vxldollar::dev::genesis_key.pub));
}
}

// Issue that tracks last changes on this test: https://github.com/vxldollarcurrency/vxldollar-node/issues/3485
// Reopen in case the nondeterministic failure appears again.
// Checks local votes (a vote with a key that is in the node's wallet) are not re-broadcast when received.
// Nodes should not relay their own votes
TEST (vote_processor, no_broadcast_local)
{
	vxldollar::system system;
	vxldollar::node_flags flags;
	flags.disable_request_loop = true;
	vxldollar::node_config config1, config2;
	config1.frontiers_confirmation = vxldollar::frontiers_confirmation_mode::disabled;
	auto & node (*system.add_node (config1, flags));
	config2.frontiers_confirmation = vxldollar::frontiers_confirmation_mode::disabled;
	config2.peering_port = vxldollar::get_available_port ();
	system.add_node (config2, flags);
	vxldollar::block_builder builder;
	std::error_code ec;
	// Reduce the weight of genesis to 2x default min voting weight
	vxldollar::keypair key;
	std::shared_ptr<vxldollar::block> send = builder.state ()
										.account (vxldollar::dev::genesis_key.pub)
										.representative (vxldollar::dev::genesis_key.pub)
										.previous (vxldollar::dev::genesis->hash ())
										.balance (2 * node.config.vote_minimum.number ())
										.link (key.pub)
										.sign (vxldollar::dev::genesis_key.prv, vxldollar::dev::genesis_key.pub)
										.work (*system.work.generate (vxldollar::dev::genesis->hash ()))
										.build (ec);
	ASSERT_FALSE (ec);
	ASSERT_EQ (vxldollar::process_result::progress, node.process_local (send).code);
	ASSERT_TIMELY (10s, !node.active.empty ());
	ASSERT_EQ (2 * node.config.vote_minimum.number (), node.weight (vxldollar::dev::genesis_key.pub));
	// Insert account in wallet. Votes on node are not enabled.
	system.wallet (0)->insert_adhoc (vxldollar::dev::genesis_key.prv);
	// Ensure that the node knows the genesis key in its wallet.
	node.wallets.compute_reps ();
	ASSERT_TRUE (node.wallets.reps ().exists (vxldollar::dev::genesis_key.pub));
	ASSERT_FALSE (node.wallets.reps ().have_half_rep ()); // Genesis balance remaining after `send' is less than the half_rep threshold
	// Process a vote with a key that is in the local wallet.
	auto vote = std::make_shared<vxldollar::vote> (vxldollar::dev::genesis_key.pub, vxldollar::dev::genesis_key.prv, vxldollar::milliseconds_since_epoch (), vxldollar::vote::duration_max, std::vector<vxldollar::block_hash>{ send->hash () });
	ASSERT_EQ (vxldollar::vote_code::vote, node.active.vote (vote));
	// Make sure the vote was processed.
	auto election (node.active.election (send->qualified_root ()));
	ASSERT_NE (nullptr, election);
	auto votes (election->votes ());
	auto existing (votes.find (vxldollar::dev::genesis_key.pub));
	ASSERT_NE (votes.end (), existing);
	ASSERT_EQ (vote->timestamp (), existing->second.timestamp);
	// Ensure the vote, from a local representative, was not broadcast on processing - it should be flooded on vote generation instead.
	ASSERT_EQ (0, node.stats.count (vxldollar::stat::type::message, vxldollar::stat::detail::confirm_ack, vxldollar::stat::dir::out));
	ASSERT_EQ (1, node.stats.count (vxldollar::stat::type::message, vxldollar::stat::detail::publish, vxldollar::stat::dir::out));
}

// Issue that tracks last changes on this test: https://github.com/vxldollarcurrency/vxldollar-node/issues/3485
// Reopen in case the nondeterministic failure appears again.
// Checks non-local votes (a vote with a key that is not in the node's wallet) are re-broadcast when received.
// Done without a representative.
TEST (vote_processor, local_broadcast_without_a_representative)
{
	vxldollar::system system;
	vxldollar::node_flags flags;
	flags.disable_request_loop = true;
	vxldollar::node_config config1, config2;
	config1.frontiers_confirmation = vxldollar::frontiers_confirmation_mode::disabled;
	auto & node (*system.add_node (config1, flags));
	config2.frontiers_confirmation = vxldollar::frontiers_confirmation_mode::disabled;
	config2.peering_port = vxldollar::get_available_port ();
	system.add_node (config2, flags);
	vxldollar::block_builder builder;
	std::error_code ec;
	// Reduce the weight of genesis to 2x default min voting weight
	vxldollar::keypair key;
	std::shared_ptr<vxldollar::block> send = builder.state ()
										.account (vxldollar::dev::genesis_key.pub)
										.representative (vxldollar::dev::genesis_key.pub)
										.previous (vxldollar::dev::genesis->hash ())
										.balance (node.config.vote_minimum.number ())
										.link (key.pub)
										.sign (vxldollar::dev::genesis_key.prv, vxldollar::dev::genesis_key.pub)
										.work (*system.work.generate (vxldollar::dev::genesis->hash ()))
										.build (ec);
	ASSERT_FALSE (ec);
	ASSERT_EQ (vxldollar::process_result::progress, node.process_local (send).code);
	ASSERT_TIMELY (10s, !node.active.empty ());
	ASSERT_EQ (node.config.vote_minimum, node.weight (vxldollar::dev::genesis_key.pub));
	node.block_confirm (send);
	// Process a vote without a representative
	auto vote = std::make_shared<vxldollar::vote> (vxldollar::dev::genesis_key.pub, vxldollar::dev::genesis_key.prv, vxldollar::milliseconds_since_epoch (), vxldollar::vote::duration_max, std::vector<vxldollar::block_hash>{ send->hash () });
	ASSERT_EQ (vxldollar::vote_code::vote, node.active.vote (vote));
	// Make sure the vote was processed.
	auto election (node.active.election (send->qualified_root ()));
	ASSERT_NE (nullptr, election);
	auto votes (election->votes ());
	auto existing (votes.find (vxldollar::dev::genesis_key.pub));
	ASSERT_NE (votes.end (), existing);
	ASSERT_EQ (vote->timestamp (), existing->second.timestamp);
	// Ensure the vote was broadcast
	ASSERT_EQ (1, node.stats.count (vxldollar::stat::type::message, vxldollar::stat::detail::confirm_ack, vxldollar::stat::dir::out));
	ASSERT_EQ (1, node.stats.count (vxldollar::stat::type::message, vxldollar::stat::detail::publish, vxldollar::stat::dir::out));
}

// Issue that tracks last changes on this test: https://github.com/vxldollarcurrency/vxldollar-node/issues/3485
// Reopen in case the nondeterministic failure appears again.
// Checks local votes (a vote with a key that is in the node's wallet) are not re-broadcast when received.
// Done with a principal representative.
TEST (vote_processor, no_broadcast_local_with_a_principal_representative)
{
	vxldollar::system system;
	vxldollar::node_flags flags;
	flags.disable_request_loop = true;
	vxldollar::node_config config1, config2;
	config1.frontiers_confirmation = vxldollar::frontiers_confirmation_mode::disabled;
	auto & node (*system.add_node (config1, flags));
	config2.frontiers_confirmation = vxldollar::frontiers_confirmation_mode::disabled;
	config2.peering_port = vxldollar::get_available_port ();
	system.add_node (config2, flags);
	vxldollar::block_builder builder;
	std::error_code ec;
	// Reduce the weight of genesis to 2x default min voting weight
	vxldollar::keypair key;
	std::shared_ptr<vxldollar::block> send = builder.state ()
										.account (vxldollar::dev::genesis_key.pub)
										.representative (vxldollar::dev::genesis_key.pub)
										.previous (vxldollar::dev::genesis->hash ())
										.balance (vxldollar::dev::constants.genesis_amount - 2 * node.config.vote_minimum.number ())
										.link (key.pub)
										.sign (vxldollar::dev::genesis_key.prv, vxldollar::dev::genesis_key.pub)
										.work (*system.work.generate (vxldollar::dev::genesis->hash ()))
										.build (ec);
	ASSERT_FALSE (ec);
	ASSERT_EQ (vxldollar::process_result::progress, node.process_local (send).code);
	ASSERT_TIMELY (10s, !node.active.empty ());
	ASSERT_EQ (vxldollar::dev::constants.genesis_amount - 2 * node.config.vote_minimum.number (), node.weight (vxldollar::dev::genesis_key.pub));
	// Insert account in wallet. Votes on node are not enabled.
	system.wallet (0)->insert_adhoc (vxldollar::dev::genesis_key.prv);
	// Ensure that the node knows the genesis key in its wallet.
	node.wallets.compute_reps ();
	ASSERT_TRUE (node.wallets.reps ().exists (vxldollar::dev::genesis_key.pub));
	ASSERT_TRUE (node.wallets.reps ().have_half_rep ()); // Genesis balance after `send' is over both half_rep and PR threshold.
	// Process a vote with a key that is in the local wallet.
	auto vote = std::make_shared<vxldollar::vote> (vxldollar::dev::genesis_key.pub, vxldollar::dev::genesis_key.prv, vxldollar::milliseconds_since_epoch (), vxldollar::vote::duration_max, std::vector<vxldollar::block_hash>{ send->hash () });
	ASSERT_EQ (vxldollar::vote_code::vote, node.active.vote (vote));
	// Make sure the vote was processed.
	auto election (node.active.election (send->qualified_root ()));
	ASSERT_NE (nullptr, election);
	auto votes (election->votes ());
	auto existing (votes.find (vxldollar::dev::genesis_key.pub));
	ASSERT_NE (votes.end (), existing);
	ASSERT_EQ (vote->timestamp (), existing->second.timestamp);
	// Ensure the vote was not broadcast.
	ASSERT_EQ (0, node.stats.count (vxldollar::stat::type::message, vxldollar::stat::detail::confirm_ack, vxldollar::stat::dir::out));
	ASSERT_EQ (1, node.stats.count (vxldollar::stat::type::message, vxldollar::stat::detail::publish, vxldollar::stat::dir::out));
}

/**
 * basic test to check that the timestamp mask is applied correctly on vote timestamp and duration fields
 */
TEST (vote, timestamp_and_duration_masking)
{
	vxldollar::system system;
	vxldollar::keypair key;
	auto hash = std::vector<vxldollar::block_hash>{ vxldollar::dev::genesis->hash () };
	auto vote = std::make_shared<vxldollar::vote> (key.pub, key.prv, 0x123f, 0xf, hash);
	ASSERT_EQ (vote->timestamp (), 0x1230);
	ASSERT_EQ (vote->duration ().count (), 524288);
	ASSERT_EQ (vote->duration_bits (), 0xf);
}