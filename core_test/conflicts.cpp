#include <vxldollar/node/election.hpp>
#include <vxldollar/test_common/system.hpp>
#include <vxldollar/test_common/testutil.hpp>

#include <gtest/gtest.h>

#include <boost/variant/get.hpp>

using namespace std::chrono_literals;

TEST (conflicts, start_stop)
{
	vxldollar::system system (1);
	auto & node1 (*system.nodes[0]);
	vxldollar::keypair key1;
	auto send1 (std::make_shared<vxldollar::send_block> (vxldollar::dev::genesis->hash (), key1.pub, 0, vxldollar::dev::genesis_key.prv, vxldollar::dev::genesis_key.pub, 0));
	node1.work_generate_blocking (*send1);
	ASSERT_EQ (vxldollar::process_result::progress, node1.process (*send1).code);
	ASSERT_EQ (0, node1.active.size ());
	node1.scheduler.activate (vxldollar::dev::genesis_key.pub, node1.store.tx_begin_read ());
	node1.scheduler.flush ();
	auto election1 = node1.active.election (send1->qualified_root ());
	ASSERT_EQ (1, node1.active.size ());
	ASSERT_NE (nullptr, election1);
	ASSERT_EQ (1, election1->votes ().size ());
}

TEST (conflicts, add_existing)
{
	vxldollar::system system (1);
	auto & node1 (*system.nodes[0]);
	vxldollar::keypair key1;
	auto send1 (std::make_shared<vxldollar::send_block> (vxldollar::dev::genesis->hash (), key1.pub, 0, vxldollar::dev::genesis_key.prv, vxldollar::dev::genesis_key.pub, 0));
	node1.work_generate_blocking (*send1);
	ASSERT_EQ (vxldollar::process_result::progress, node1.process (*send1).code);
	node1.scheduler.activate (vxldollar::dev::genesis_key.pub, node1.store.tx_begin_read ());
	vxldollar::keypair key2;
	auto send2 (std::make_shared<vxldollar::send_block> (vxldollar::dev::genesis->hash (), key2.pub, 0, vxldollar::dev::genesis_key.prv, vxldollar::dev::genesis_key.pub, 0));
	send2->sideband_set ({});
	node1.scheduler.activate (vxldollar::dev::genesis_key.pub, node1.store.tx_begin_read ());
	node1.scheduler.flush ();
	auto election1 = node1.active.election (send2->qualified_root ());
	ASSERT_NE (nullptr, election1);
	ASSERT_EQ (1, node1.active.size ());
	auto vote1 (std::make_shared<vxldollar::vote> (key2.pub, key2.prv, 0, 0, send2));
	node1.active.vote (vote1);
	ASSERT_EQ (2, election1->votes ().size ());
	auto votes (election1->votes ());
	ASSERT_NE (votes.end (), votes.find (key2.pub));
}

TEST (conflicts, add_two)
{
	vxldollar::system system{};
	auto const & node = system.add_node ();

	system.wallet (0)->insert_adhoc (vxldollar::dev::genesis_key.prv);

	// define a functor that sends from given account to given destination,
	// optionally force-confirming the send blocks *and* receiving on the destination account;
	// the functor returns a pair of the send and receive blocks created or nullptrs if something failed
	//
	auto const do_send = [&node, &system] (auto const & previous, auto const & from, auto const & to, bool forceConfirm = true)
	-> std::pair<std::optional<std::shared_ptr<vxldollar::block>>, std::optional<std::shared_ptr<vxldollar::block>>> {
		auto const send = vxldollar::send_block_builder{}.make_block ().previous (previous).destination (to.pub).balance (0).sign (from.prv, from.pub).work (*system.work.generate (previous)).build_shared ();

		if (vxldollar::process_result::progress != node->process (*send).code)
		{
			return std::make_pair (std::nullopt, std::nullopt);
		}

		if (!forceConfirm)
		{
			return std::make_pair (std::move (send), std::nullopt);
		}

		auto const is_confirmed = [&node] (auto const & hash) {
			return node->block_confirmed (hash);
		};

		node->process_confirmed (vxldollar::election_status{ send });
		auto const is_send_not_confirmed = system.poll_until_true (5s, std::bind (is_confirmed, send->hash ()));
		if (is_send_not_confirmed)
		{
			return std::make_pair (std::nullopt, std::nullopt);
		}

		auto const receive = vxldollar::open_block_builder{}.make_block ().account (to.pub).source (send->hash ()).representative (to.pub).sign (to.prv, to.pub).work (*system.work.generate (to.pub)).build_shared ();

		if (vxldollar::process_result::progress != node->process (*receive).code)
		{
			return std::make_pair (std::nullopt, std::nullopt);
		}

		node->process_confirmed (vxldollar::election_status{ receive });
		auto const is_receive_not_confirmed = system.poll_until_true (5s, std::bind (is_confirmed, receive->hash ()));
		if (is_receive_not_confirmed)
		{
			return std::make_pair (std::move (send), std::nullopt);
		}

		return std::make_pair (std::move (send), std::move (receive));
	};

	// send from genesis to account1 and receive it on account1
	//
	vxldollar::keypair account1{};
	auto const [send1, receive1] = do_send (vxldollar::dev::genesis->hash (), vxldollar::dev::genesis_key, account1);
	ASSERT_TRUE (send1.has_value () && receive1.has_value ());
	// both blocks having been fully confirmed, we expect 1 (genesis) + 2 (send/receive) = 3 cemented blocks
	//
	ASSERT_EQ (3, node->ledger.cache.cemented_count);

	vxldollar::keypair account2{};
	auto const [send2, receive2] = do_send ((*send1)->hash (), vxldollar::dev::genesis_key, account2);
	ASSERT_TRUE (send2.has_value () && receive2.has_value ());
	ASSERT_EQ (5, node->ledger.cache.cemented_count);

	// send from account1 to account3 but do not receive it on account3 and do not force-confirm the send block
	//
	vxldollar::keypair account3{};
	auto const [send3, dummy1] = do_send ((*receive1)->hash (), account1, account3, false);
	ASSERT_TRUE (send3.has_value ());
	// expect the number of cemented blocks not to have changed since the last operation
	//
	ASSERT_EQ (5, node->ledger.cache.cemented_count);

	auto const [send4, dummy2] = do_send ((*receive2)->hash (), account2, account3, false);
	ASSERT_TRUE (send4.has_value ());
	ASSERT_EQ (5, node->ledger.cache.cemented_count);

	// activate elections for the previous two send blocks (to account3) that we did not forcefully confirm
	//
	node->scheduler.activate (account3.pub, node->store.tx_begin_read ());
	ASSERT_TIMELY (5s, node->active.election ((*send3)->qualified_root ()) != nullptr);
	ASSERT_TIMELY (5s, node->active.election ((*send4)->qualified_root ()) != nullptr);

	// wait 3s before asserting just to make sure there would be enough time
	// for the Active Elections Container to evict both elections in case they would wrongfully get confirmed
	//
	ASSERT_TIMELY (5s, node->active.size () == 2);
}

TEST (vote_uniquer, null)
{
	vxldollar::block_uniquer block_uniquer;
	vxldollar::vote_uniquer uniquer (block_uniquer);
	ASSERT_EQ (nullptr, uniquer.unique (nullptr));
}

// Show that an identical vote can be uniqued
TEST (vote_uniquer, same_vote)
{
	vxldollar::block_uniquer block_uniquer;
	vxldollar::vote_uniquer uniquer (block_uniquer);
	vxldollar::keypair key;
	auto vote1 (std::make_shared<vxldollar::vote> (key.pub, key.prv, 0, 0, std::make_shared<vxldollar::state_block> (0, 0, 0, 0, 0, key.prv, key.pub, 0)));
	auto vote2 (std::make_shared<vxldollar::vote> (*vote1));
	ASSERT_EQ (vote1, uniquer.unique (vote1));
	ASSERT_EQ (vote1, uniquer.unique (vote2));
}

// Show that a different vote for the same block will have the block uniqued
TEST (vote_uniquer, same_block)
{
	vxldollar::block_uniquer block_uniquer;
	vxldollar::vote_uniquer uniquer (block_uniquer);
	vxldollar::keypair key1;
	vxldollar::keypair key2;
	auto block1 (std::make_shared<vxldollar::state_block> (0, 0, 0, 0, 0, key1.prv, key1.pub, 0));
	auto block2 (std::make_shared<vxldollar::state_block> (*block1));
	auto vote1 (std::make_shared<vxldollar::vote> (key1.pub, key1.prv, 0, 0, block1));
	auto vote2 (std::make_shared<vxldollar::vote> (key1.pub, key1.prv, 0, 0, block2));
	ASSERT_EQ (vote1, uniquer.unique (vote1));
	ASSERT_EQ (vote2, uniquer.unique (vote2));
	ASSERT_NE (vote1, vote2);
	ASSERT_EQ (boost::get<std::shared_ptr<vxldollar::block>> (vote1->blocks[0]), boost::get<std::shared_ptr<vxldollar::block>> (vote2->blocks[0]));
}

TEST (vote_uniquer, vbh_one)
{
	vxldollar::block_uniquer block_uniquer;
	vxldollar::vote_uniquer uniquer (block_uniquer);
	vxldollar::keypair key;
	auto block (std::make_shared<vxldollar::state_block> (0, 0, 0, 0, 0, key.prv, key.pub, 0));
	std::vector<vxldollar::block_hash> hashes;
	hashes.push_back (block->hash ());
	auto vote1 (std::make_shared<vxldollar::vote> (key.pub, key.prv, 0, 0, hashes));
	auto vote2 (std::make_shared<vxldollar::vote> (*vote1));
	ASSERT_EQ (vote1, uniquer.unique (vote1));
	ASSERT_EQ (vote1, uniquer.unique (vote2));
}

TEST (vote_uniquer, vbh_two)
{
	vxldollar::block_uniquer block_uniquer;
	vxldollar::vote_uniquer uniquer (block_uniquer);
	vxldollar::keypair key;
	auto block1 (std::make_shared<vxldollar::state_block> (0, 0, 0, 0, 0, key.prv, key.pub, 0));
	std::vector<vxldollar::block_hash> hashes1;
	hashes1.push_back (block1->hash ());
	auto block2 (std::make_shared<vxldollar::state_block> (1, 0, 0, 0, 0, key.prv, key.pub, 0));
	std::vector<vxldollar::block_hash> hashes2;
	hashes2.push_back (block2->hash ());
	auto vote1 (std::make_shared<vxldollar::vote> (key.pub, key.prv, 0, 0, hashes1));
	auto vote2 (std::make_shared<vxldollar::vote> (key.pub, key.prv, 0, 0, hashes2));
	ASSERT_EQ (vote1, uniquer.unique (vote1));
	ASSERT_EQ (vote2, uniquer.unique (vote2));
}

TEST (vote_uniquer, cleanup)
{
	vxldollar::block_uniquer block_uniquer;
	vxldollar::vote_uniquer uniquer (block_uniquer);
	vxldollar::keypair key;
	auto vote1 (std::make_shared<vxldollar::vote> (key.pub, key.prv, 0, 0, std::make_shared<vxldollar::state_block> (0, 0, 0, 0, 0, key.prv, key.pub, 0)));
	auto vote2 (std::make_shared<vxldollar::vote> (key.pub, key.prv, vxldollar::vote::timestamp_min * 1, 0, std::make_shared<vxldollar::state_block> (0, 0, 0, 0, 0, key.prv, key.pub, 0)));
	auto vote3 (uniquer.unique (vote1));
	auto vote4 (uniquer.unique (vote2));
	vote2.reset ();
	vote4.reset ();
	ASSERT_EQ (2, uniquer.size ());
	auto iterations (0);
	while (uniquer.size () == 2)
	{
		auto vote5 (uniquer.unique (vote1));
		ASSERT_LT (iterations++, 200);
	}
}
