#include <vxldollar/node/common.hpp>
#include <vxldollar/node/network.hpp>
#include <vxldollar/secure/buffer.hpp>

#include <gtest/gtest.h>

#include <boost/algorithm/string.hpp>
#include <boost/variant/get.hpp>

TEST (message, keepalive_serialization)
{
	vxldollar::keepalive request1{ vxldollar::dev::network_params.network };
	std::vector<uint8_t> bytes;
	{
		vxldollar::vectorstream stream (bytes);
		request1.serialize (stream);
	}
	auto error (false);
	vxldollar::bufferstream stream (bytes.data (), bytes.size ());
	vxldollar::message_header header (error, stream);
	ASSERT_FALSE (error);
	vxldollar::keepalive request2 (error, stream, header);
	ASSERT_FALSE (error);
	ASSERT_EQ (request1, request2);
}

TEST (message, keepalive_deserialize)
{
	vxldollar::keepalive message1{ vxldollar::dev::network_params.network };
	message1.peers[0] = vxldollar::endpoint (boost::asio::ip::address_v6::loopback (), 10000);
	std::vector<uint8_t> bytes;
	{
		vxldollar::vectorstream stream (bytes);
		message1.serialize (stream);
	}
	vxldollar::bufferstream stream (bytes.data (), bytes.size ());
	auto error (false);
	vxldollar::message_header header (error, stream);
	ASSERT_FALSE (error);
	ASSERT_EQ (vxldollar::message_type::keepalive, header.type);
	vxldollar::keepalive message2 (error, stream, header);
	ASSERT_FALSE (error);
	ASSERT_EQ (message1.peers, message2.peers);
}

TEST (message, publish_serialization)
{
	vxldollar::publish publish{ vxldollar::dev::network_params.network, std::make_shared<vxldollar::send_block> (0, 1, 2, vxldollar::keypair ().prv, 4, 5) };
	ASSERT_EQ (vxldollar::block_type::send, publish.header.block_type ());
	std::vector<uint8_t> bytes;
	{
		vxldollar::vectorstream stream (bytes);
		publish.header.serialize (stream);
	}
	ASSERT_EQ (8, bytes.size ());
	ASSERT_EQ (0x52, bytes[0]);
	ASSERT_EQ (0x41, bytes[1]);
	ASSERT_EQ (vxldollar::dev::network_params.network.protocol_version, bytes[2]);
	ASSERT_EQ (vxldollar::dev::network_params.network.protocol_version, bytes[3]);
	ASSERT_EQ (vxldollar::dev::network_params.network.protocol_version_min, bytes[4]);
	ASSERT_EQ (static_cast<uint8_t> (vxldollar::message_type::publish), bytes[5]);
	ASSERT_EQ (0x00, bytes[6]); // extensions
	ASSERT_EQ (static_cast<uint8_t> (vxldollar::block_type::send), bytes[7]);
	vxldollar::bufferstream stream (bytes.data (), bytes.size ());
	auto error (false);
	vxldollar::message_header header (error, stream);
	ASSERT_FALSE (error);
	ASSERT_EQ (vxldollar::dev::network_params.network.protocol_version_min, header.version_min);
	ASSERT_EQ (vxldollar::dev::network_params.network.protocol_version, header.version_using);
	ASSERT_EQ (vxldollar::dev::network_params.network.protocol_version, header.version_max);
	ASSERT_EQ (vxldollar::message_type::publish, header.type);
}

TEST (message, confirm_ack_serialization)
{
	vxldollar::keypair key1;
	auto vote (std::make_shared<vxldollar::vote> (key1.pub, key1.prv, 0, 0, std::make_shared<vxldollar::send_block> (0, 1, 2, key1.prv, 4, 5)));
	vxldollar::confirm_ack con1{ vxldollar::dev::network_params.network, vote };
	std::vector<uint8_t> bytes;
	{
		vxldollar::vectorstream stream1 (bytes);
		con1.serialize (stream1);
	}
	vxldollar::bufferstream stream2 (bytes.data (), bytes.size ());
	bool error (false);
	vxldollar::message_header header (error, stream2);
	vxldollar::confirm_ack con2 (error, stream2, header);
	ASSERT_FALSE (error);
	ASSERT_EQ (con1, con2);
	ASSERT_EQ (header.block_type (), vxldollar::block_type::send);
}

TEST (message, confirm_ack_hash_serialization)
{
	std::vector<vxldollar::block_hash> hashes;
	for (auto i (hashes.size ()); i < vxldollar::network::confirm_ack_hashes_max; i++)
	{
		vxldollar::keypair key1;
		vxldollar::block_hash previous;
		vxldollar::random_pool::generate_block (previous.bytes.data (), previous.bytes.size ());
		vxldollar::state_block block (key1.pub, previous, key1.pub, 2, 4, key1.prv, key1.pub, 5);
		hashes.push_back (block.hash ());
	}
	vxldollar::keypair representative1;
	auto vote (std::make_shared<vxldollar::vote> (representative1.pub, representative1.prv, 0, 0, hashes));
	vxldollar::confirm_ack con1{ vxldollar::dev::network_params.network, vote };
	std::vector<uint8_t> bytes;
	{
		vxldollar::vectorstream stream1 (bytes);
		con1.serialize (stream1);
	}
	vxldollar::bufferstream stream2 (bytes.data (), bytes.size ());
	bool error (false);
	vxldollar::message_header header (error, stream2);
	vxldollar::confirm_ack con2 (error, stream2, header);
	ASSERT_FALSE (error);
	ASSERT_EQ (con1, con2);
	std::vector<vxldollar::block_hash> vote_blocks;
	for (auto block : con2.vote->blocks)
	{
		vote_blocks.push_back (boost::get<vxldollar::block_hash> (block));
	}
	ASSERT_EQ (hashes, vote_blocks);
	// Check overflow with max hashes
	ASSERT_EQ (header.count_get (), hashes.size ());
	ASSERT_EQ (header.block_type (), vxldollar::block_type::not_a_block);
}

TEST (message, confirm_req_serialization)
{
	vxldollar::keypair key1;
	vxldollar::keypair key2;
	auto block (std::make_shared<vxldollar::send_block> (0, key2.pub, 200, vxldollar::keypair ().prv, 2, 3));
	vxldollar::confirm_req req{ vxldollar::dev::network_params.network, block };
	std::vector<uint8_t> bytes;
	{
		vxldollar::vectorstream stream (bytes);
		req.serialize (stream);
	}
	auto error (false);
	vxldollar::bufferstream stream2 (bytes.data (), bytes.size ());
	vxldollar::message_header header (error, stream2);
	vxldollar::confirm_req req2 (error, stream2, header);
	ASSERT_FALSE (error);
	ASSERT_EQ (req, req2);
	ASSERT_EQ (*req.block, *req2.block);
}

TEST (message, confirm_req_hash_serialization)
{
	vxldollar::keypair key1;
	vxldollar::keypair key2;
	vxldollar::send_block block (1, key2.pub, 200, vxldollar::keypair ().prv, 2, 3);
	vxldollar::confirm_req req{ vxldollar::dev::network_params.network, block.hash (), block.root () };
	std::vector<uint8_t> bytes;
	{
		vxldollar::vectorstream stream (bytes);
		req.serialize (stream);
	}
	auto error (false);
	vxldollar::bufferstream stream2 (bytes.data (), bytes.size ());
	vxldollar::message_header header (error, stream2);
	vxldollar::confirm_req req2 (error, stream2, header);
	ASSERT_FALSE (error);
	ASSERT_EQ (req, req2);
	ASSERT_EQ (req.roots_hashes, req2.roots_hashes);
	ASSERT_EQ (header.block_type (), vxldollar::block_type::not_a_block);
	ASSERT_EQ (header.count_get (), req.roots_hashes.size ());
}

TEST (message, confirm_req_hash_batch_serialization)
{
	vxldollar::keypair key;
	vxldollar::keypair representative;
	std::vector<std::pair<vxldollar::block_hash, vxldollar::root>> roots_hashes;
	vxldollar::state_block open (key.pub, 0, representative.pub, 2, 4, key.prv, key.pub, 5);
	roots_hashes.push_back (std::make_pair (open.hash (), open.root ()));
	for (auto i (roots_hashes.size ()); i < 7; i++)
	{
		vxldollar::keypair key1;
		vxldollar::block_hash previous;
		vxldollar::random_pool::generate_block (previous.bytes.data (), previous.bytes.size ());
		vxldollar::state_block block (key1.pub, previous, representative.pub, 2, 4, key1.prv, key1.pub, 5);
		roots_hashes.push_back (std::make_pair (block.hash (), block.root ()));
	}
	roots_hashes.push_back (std::make_pair (open.hash (), open.root ()));
	vxldollar::confirm_req req{ vxldollar::dev::network_params.network, roots_hashes };
	std::vector<uint8_t> bytes;
	{
		vxldollar::vectorstream stream (bytes);
		req.serialize (stream);
	}
	auto error (false);
	vxldollar::bufferstream stream2 (bytes.data (), bytes.size ());
	vxldollar::message_header header (error, stream2);
	vxldollar::confirm_req req2 (error, stream2, header);
	ASSERT_FALSE (error);
	ASSERT_EQ (req, req2);
	ASSERT_EQ (req.roots_hashes, req2.roots_hashes);
	ASSERT_EQ (req.roots_hashes, roots_hashes);
	ASSERT_EQ (req2.roots_hashes, roots_hashes);
	ASSERT_EQ (header.block_type (), vxldollar::block_type::not_a_block);
	ASSERT_EQ (header.count_get (), req.roots_hashes.size ());
}

// this unit test checks that conversion of message_header to string works as expected
TEST (message, message_header_to_string)
{
	// calculate expected string
	int maxver = vxldollar::dev::network_params.network.protocol_version;
	int minver = vxldollar::dev::network_params.network.protocol_version_min;
	std::stringstream ss;
	ss << "NetID: 5241(dev), VerMaxUsingMin: " << maxver << "/" << maxver << "/" << minver << ", MsgType: 2(keepalive), Extensions: 0000";
	auto expected_str = ss.str ();

	// check expected vs real
	vxldollar::keepalive keepalive_msg{ vxldollar::dev::network_params.network };
	std::string header_string = keepalive_msg.header.to_string ();
	ASSERT_EQ (expected_str, header_string);
}
