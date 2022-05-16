#include <vxldollar/lib/jsonconfig.hpp>
#include <vxldollar/node/confirmation_solicitor.hpp>
#include <vxldollar/test_common/system.hpp>
#include <vxldollar/test_common/testutil.hpp>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

TEST (confirmation_solicitor, batches)
{
	vxldollar::system system;
	vxldollar::node_flags node_flags;
	node_flags.disable_request_loop = true;
	node_flags.disable_rep_crawler = true;
	node_flags.disable_udp = false;
	auto & node1 = *system.add_node (node_flags);
	node_flags.disable_request_loop = true;
	auto & node2 = *system.add_node (node_flags);
	auto channel1 (node2.network.udp_channels.create (node1.network.endpoint ()));
	// Solicitor will only solicit from this representative
	vxldollar::representative representative (vxldollar::dev::genesis_key.pub, vxldollar::dev::constants.genesis_amount, channel1);
	std::vector<vxldollar::representative> representatives{ representative };
	vxldollar::confirmation_solicitor solicitor (node2.network, node2.config);
	solicitor.prepare (representatives);
	// Ensure the representatives are correct
	ASSERT_EQ (1, representatives.size ());
	ASSERT_EQ (channel1, representatives.front ().channel);
	ASSERT_EQ (vxldollar::dev::genesis_key.pub, representatives.front ().account);
	ASSERT_TIMELY (3s, node2.network.size () == 1);
	auto send (std::make_shared<vxldollar::send_block> (vxldollar::dev::genesis->hash (), vxldollar::keypair ().pub, vxldollar::dev::constants.genesis_amount - 100, vxldollar::dev::genesis_key.prv, vxldollar::dev::genesis_key.pub, *system.work.generate (vxldollar::dev::genesis->hash ())));
	send->sideband_set ({});
	{
		vxldollar::lock_guard<vxldollar::mutex> guard (node2.active.mutex);
		for (size_t i (0); i < vxldollar::network::confirm_req_hashes_max; ++i)
		{
			auto election (std::make_shared<vxldollar::election> (node2, send, nullptr, nullptr, vxldollar::election_behavior::normal));
			ASSERT_FALSE (solicitor.add (*election));
		}
		// Reached the maximum amount of requests for the channel
		auto election (std::make_shared<vxldollar::election> (node2, send, nullptr, nullptr, vxldollar::election_behavior::normal));
		ASSERT_TRUE (solicitor.add (*election));
		// Broadcasting should be immediate
		ASSERT_EQ (0, node2.stats.count (vxldollar::stat::type::message, vxldollar::stat::detail::publish, vxldollar::stat::dir::out));
		ASSERT_FALSE (solicitor.broadcast (*election));
	}
	// One publish through directed broadcasting and another through random flooding
	ASSERT_EQ (2, node2.stats.count (vxldollar::stat::type::message, vxldollar::stat::detail::publish, vxldollar::stat::dir::out));
	solicitor.flush ();
	ASSERT_EQ (1, node2.stats.count (vxldollar::stat::type::message, vxldollar::stat::detail::confirm_req, vxldollar::stat::dir::out));
}

namespace vxldollar
{
TEST (confirmation_solicitor, different_hash)
{
	vxldollar::system system;
	vxldollar::node_flags node_flags;
	node_flags.disable_request_loop = true;
	node_flags.disable_rep_crawler = true;
	node_flags.disable_udp = false;
	auto & node1 = *system.add_node (node_flags);
	auto & node2 = *system.add_node (node_flags);
	auto channel1 (node2.network.udp_channels.create (node1.network.endpoint ()));
	// Solicitor will only solicit from this representative
	vxldollar::representative representative (vxldollar::dev::genesis_key.pub, vxldollar::dev::constants.genesis_amount, channel1);
	std::vector<vxldollar::representative> representatives{ representative };
	vxldollar::confirmation_solicitor solicitor (node2.network, node2.config);
	solicitor.prepare (representatives);
	// Ensure the representatives are correct
	ASSERT_EQ (1, representatives.size ());
	ASSERT_EQ (channel1, representatives.front ().channel);
	ASSERT_EQ (vxldollar::dev::genesis_key.pub, representatives.front ().account);
	ASSERT_TIMELY (3s, node2.network.size () == 1);
	auto send (std::make_shared<vxldollar::send_block> (vxldollar::dev::genesis->hash (), vxldollar::keypair ().pub, vxldollar::dev::constants.genesis_amount - 100, vxldollar::dev::genesis_key.prv, vxldollar::dev::genesis_key.pub, *system.work.generate (vxldollar::dev::genesis->hash ())));
	send->sideband_set ({});
	auto election (std::make_shared<vxldollar::election> (node2, send, nullptr, nullptr, vxldollar::election_behavior::normal));
	// Add a vote for something else, not the winner
	election->last_votes[representative.account] = { std::chrono::steady_clock::now (), 1, 1 };
	// Ensure the request and broadcast goes through
	ASSERT_FALSE (solicitor.add (*election));
	ASSERT_FALSE (solicitor.broadcast (*election));
	// One publish through directed broadcasting and another through random flooding
	ASSERT_EQ (2, node2.stats.count (vxldollar::stat::type::message, vxldollar::stat::detail::publish, vxldollar::stat::dir::out));
	solicitor.flush ();
	ASSERT_EQ (1, node2.stats.count (vxldollar::stat::type::message, vxldollar::stat::detail::confirm_req, vxldollar::stat::dir::out));
}

TEST (confirmation_solicitor, bypass_max_requests_cap)
{
	vxldollar::system system;
	vxldollar::node_flags node_flags;
	node_flags.disable_request_loop = true;
	node_flags.disable_rep_crawler = true;
	node_flags.disable_udp = false;
	auto & node1 = *system.add_node (node_flags);
	auto & node2 = *system.add_node (node_flags);
	vxldollar::confirmation_solicitor solicitor (node2.network, node2.config);
	std::vector<vxldollar::representative> representatives;
	auto max_representatives = std::max<size_t> (solicitor.max_election_requests, solicitor.max_election_broadcasts);
	representatives.reserve (max_representatives + 1);
	for (auto i (0); i < max_representatives + 1; ++i)
	{
		auto channel (node2.network.udp_channels.create (node1.network.endpoint ()));
		vxldollar::representative representative (vxldollar::account (i), i, channel);
		representatives.push_back (representative);
	}
	ASSERT_EQ (max_representatives + 1, representatives.size ());
	solicitor.prepare (representatives);
	ASSERT_TIMELY (3s, node2.network.size () == 1);
	auto send (std::make_shared<vxldollar::send_block> (vxldollar::dev::genesis->hash (), vxldollar::keypair ().pub, vxldollar::dev::constants.genesis_amount - 100, vxldollar::dev::genesis_key.prv, vxldollar::dev::genesis_key.pub, *system.work.generate (vxldollar::dev::genesis->hash ())));
	send->sideband_set ({});
	auto election (std::make_shared<vxldollar::election> (node2, send, nullptr, nullptr, vxldollar::election_behavior::normal));
	// Add a vote for something else, not the winner
	for (auto const & rep : representatives)
	{
		vxldollar::lock_guard<vxldollar::mutex> guard (election->mutex);
		election->last_votes[rep.account] = { std::chrono::steady_clock::now (), 1, 1 };
	}
	ASSERT_FALSE (solicitor.add (*election));
	ASSERT_FALSE (solicitor.broadcast (*election));
	solicitor.flush ();
	// All requests went through, the last one would normally not go through due to the cap but a vote for a different hash does not count towards the cap
	ASSERT_EQ (max_representatives + 1, node2.stats.count (vxldollar::stat::type::message, vxldollar::stat::detail::confirm_req, vxldollar::stat::dir::out));

	solicitor.prepare (representatives);
	auto election2 (std::make_shared<vxldollar::election> (node2, send, nullptr, nullptr, vxldollar::election_behavior::normal));
	ASSERT_FALSE (solicitor.add (*election2));
	ASSERT_FALSE (solicitor.broadcast (*election2));

	solicitor.flush ();

	// All requests but one went through, due to the cap
	ASSERT_EQ (2 * max_representatives + 1, node2.stats.count (vxldollar::stat::type::message, vxldollar::stat::detail::confirm_req, vxldollar::stat::dir::out));
}
}
