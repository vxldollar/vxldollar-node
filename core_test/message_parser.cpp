#include <vxldollar/test_common/system.hpp>
#include <vxldollar/test_common/testutil.hpp>

#include <gtest/gtest.h>

namespace
{
class dev_visitor : public vxldollar::message_visitor
{
public:
	void keepalive (vxldollar::keepalive const &) override
	{
		++keepalive_count;
	}
	void publish (vxldollar::publish const &) override
	{
		++publish_count;
	}
	void confirm_req (vxldollar::confirm_req const &) override
	{
		++confirm_req_count;
	}
	void confirm_ack (vxldollar::confirm_ack const &) override
	{
		++confirm_ack_count;
	}
	void bulk_pull (vxldollar::bulk_pull const &) override
	{
		ASSERT_FALSE (true);
	}
	void bulk_pull_account (vxldollar::bulk_pull_account const &) override
	{
		ASSERT_FALSE (true);
	}
	void bulk_push (vxldollar::bulk_push const &) override
	{
		ASSERT_FALSE (true);
	}
	void frontier_req (vxldollar::frontier_req const &) override
	{
		ASSERT_FALSE (true);
	}
	void node_id_handshake (vxldollar::node_id_handshake const &) override
	{
		ASSERT_FALSE (true);
	}
	void telemetry_req (vxldollar::telemetry_req const &) override
	{
		ASSERT_FALSE (true);
	}
	void telemetry_ack (vxldollar::telemetry_ack const &) override
	{
		ASSERT_FALSE (true);
	}

	uint64_t keepalive_count{ 0 };
	uint64_t publish_count{ 0 };
	uint64_t confirm_req_count{ 0 };
	uint64_t confirm_ack_count{ 0 };
};
}

TEST (message_parser, exact_confirm_ack_size)
{
	vxldollar::system system (1);
	dev_visitor visitor;
	vxldollar::network_filter filter (1);
	vxldollar::block_uniquer block_uniquer;
	vxldollar::vote_uniquer vote_uniquer (block_uniquer);
	vxldollar::message_parser parser (filter, block_uniquer, vote_uniquer, visitor, system.work, vxldollar::dev::network_params.network);
	auto block (std::make_shared<vxldollar::send_block> (1, 1, 2, vxldollar::keypair ().prv, 4, *system.work.generate (vxldollar::root (1))));
	auto vote (std::make_shared<vxldollar::vote> (0, vxldollar::keypair ().prv, 0, 0, std::move (block)));
	vxldollar::confirm_ack message{ vxldollar::dev::network_params.network, vote };
	std::vector<uint8_t> bytes;
	{
		vxldollar::vectorstream stream (bytes);
		message.serialize (stream);
	}
	ASSERT_EQ (0, visitor.confirm_ack_count);
	ASSERT_EQ (parser.status, vxldollar::message_parser::parse_status::success);
	auto error (false);
	vxldollar::bufferstream stream1 (bytes.data (), bytes.size ());
	vxldollar::message_header header1 (error, stream1);
	ASSERT_FALSE (error);
	parser.deserialize_confirm_ack (stream1, header1);
	ASSERT_EQ (1, visitor.confirm_ack_count);
	ASSERT_EQ (parser.status, vxldollar::message_parser::parse_status::success);
	bytes.push_back (0);
	vxldollar::bufferstream stream2 (bytes.data (), bytes.size ());
	vxldollar::message_header header2 (error, stream2);
	ASSERT_FALSE (error);
	parser.deserialize_confirm_ack (stream2, header2);
	ASSERT_EQ (1, visitor.confirm_ack_count);
	ASSERT_NE (parser.status, vxldollar::message_parser::parse_status::success);
}

TEST (message_parser, exact_confirm_req_size)
{
	vxldollar::system system (1);
	dev_visitor visitor;
	vxldollar::network_filter filter (1);
	vxldollar::block_uniquer block_uniquer;
	vxldollar::vote_uniquer vote_uniquer (block_uniquer);
	vxldollar::message_parser parser (filter, block_uniquer, vote_uniquer, visitor, system.work, vxldollar::dev::network_params.network);
	auto block (std::make_shared<vxldollar::send_block> (1, 1, 2, vxldollar::keypair ().prv, 4, *system.work.generate (vxldollar::root (1))));
	vxldollar::confirm_req message{ vxldollar::dev::network_params.network, block };
	std::vector<uint8_t> bytes;
	{
		vxldollar::vectorstream stream (bytes);
		message.serialize (stream);
	}
	ASSERT_EQ (0, visitor.confirm_req_count);
	ASSERT_EQ (parser.status, vxldollar::message_parser::parse_status::success);
	auto error (false);
	vxldollar::bufferstream stream1 (bytes.data (), bytes.size ());
	vxldollar::message_header header1 (error, stream1);
	ASSERT_FALSE (error);
	parser.deserialize_confirm_req (stream1, header1);
	ASSERT_EQ (1, visitor.confirm_req_count);
	ASSERT_EQ (parser.status, vxldollar::message_parser::parse_status::success);
	bytes.push_back (0);
	vxldollar::bufferstream stream2 (bytes.data (), bytes.size ());
	vxldollar::message_header header2 (error, stream2);
	ASSERT_FALSE (error);
	parser.deserialize_confirm_req (stream2, header2);
	ASSERT_EQ (1, visitor.confirm_req_count);
	ASSERT_NE (parser.status, vxldollar::message_parser::parse_status::success);
}

TEST (message_parser, exact_confirm_req_hash_size)
{
	vxldollar::system system (1);
	dev_visitor visitor;
	vxldollar::network_filter filter (1);
	vxldollar::block_uniquer block_uniquer;
	vxldollar::vote_uniquer vote_uniquer (block_uniquer);
	vxldollar::message_parser parser (filter, block_uniquer, vote_uniquer, visitor, system.work, vxldollar::dev::network_params.network);
	vxldollar::send_block block (1, 1, 2, vxldollar::keypair ().prv, 4, *system.work.generate (vxldollar::root (1)));
	vxldollar::confirm_req message{ vxldollar::dev::network_params.network, block.hash (), block.root () };
	std::vector<uint8_t> bytes;
	{
		vxldollar::vectorstream stream (bytes);
		message.serialize (stream);
	}
	ASSERT_EQ (0, visitor.confirm_req_count);
	ASSERT_EQ (parser.status, vxldollar::message_parser::parse_status::success);
	auto error (false);
	vxldollar::bufferstream stream1 (bytes.data (), bytes.size ());
	vxldollar::message_header header1 (error, stream1);
	ASSERT_FALSE (error);
	parser.deserialize_confirm_req (stream1, header1);
	ASSERT_EQ (1, visitor.confirm_req_count);
	ASSERT_EQ (parser.status, vxldollar::message_parser::parse_status::success);
	bytes.push_back (0);
	vxldollar::bufferstream stream2 (bytes.data (), bytes.size ());
	vxldollar::message_header header2 (error, stream2);
	ASSERT_FALSE (error);
	parser.deserialize_confirm_req (stream2, header2);
	ASSERT_EQ (1, visitor.confirm_req_count);
	ASSERT_NE (parser.status, vxldollar::message_parser::parse_status::success);
}

TEST (message_parser, exact_publish_size)
{
	vxldollar::system system (1);
	dev_visitor visitor;
	vxldollar::network_filter filter (1);
	vxldollar::block_uniquer block_uniquer;
	vxldollar::vote_uniquer vote_uniquer (block_uniquer);
	vxldollar::message_parser parser (filter, block_uniquer, vote_uniquer, visitor, system.work, vxldollar::dev::network_params.network);
	auto block (std::make_shared<vxldollar::send_block> (1, 1, 2, vxldollar::keypair ().prv, 4, *system.work.generate (vxldollar::root (1))));
	vxldollar::publish message{ vxldollar::dev::network_params.network, block };
	std::vector<uint8_t> bytes;
	{
		vxldollar::vectorstream stream (bytes);
		message.serialize (stream);
	}
	ASSERT_EQ (0, visitor.publish_count);
	ASSERT_EQ (parser.status, vxldollar::message_parser::parse_status::success);
	auto error (false);
	vxldollar::bufferstream stream1 (bytes.data (), bytes.size ());
	vxldollar::message_header header1 (error, stream1);
	ASSERT_FALSE (error);
	parser.deserialize_publish (stream1, header1);
	ASSERT_EQ (1, visitor.publish_count);
	ASSERT_EQ (parser.status, vxldollar::message_parser::parse_status::success);
	bytes.push_back (0);
	vxldollar::bufferstream stream2 (bytes.data (), bytes.size ());
	vxldollar::message_header header2 (error, stream2);
	ASSERT_FALSE (error);
	parser.deserialize_publish (stream2, header2);
	ASSERT_EQ (1, visitor.publish_count);
	ASSERT_NE (parser.status, vxldollar::message_parser::parse_status::success);
}

TEST (message_parser, exact_keepalive_size)
{
	vxldollar::system system (1);
	dev_visitor visitor;
	vxldollar::network_filter filter (1);
	vxldollar::block_uniquer block_uniquer;
	vxldollar::vote_uniquer vote_uniquer (block_uniquer);
	vxldollar::message_parser parser (filter, block_uniquer, vote_uniquer, visitor, system.work, vxldollar::dev::network_params.network);
	vxldollar::keepalive message{ vxldollar::dev::network_params.network };
	std::vector<uint8_t> bytes;
	{
		vxldollar::vectorstream stream (bytes);
		message.serialize (stream);
	}
	ASSERT_EQ (0, visitor.keepalive_count);
	ASSERT_EQ (parser.status, vxldollar::message_parser::parse_status::success);
	auto error (false);
	vxldollar::bufferstream stream1 (bytes.data (), bytes.size ());
	vxldollar::message_header header1 (error, stream1);
	ASSERT_FALSE (error);
	parser.deserialize_keepalive (stream1, header1);
	ASSERT_EQ (1, visitor.keepalive_count);
	ASSERT_EQ (parser.status, vxldollar::message_parser::parse_status::success);
	bytes.push_back (0);
	vxldollar::bufferstream stream2 (bytes.data (), bytes.size ());
	vxldollar::message_header header2 (error, stream2);
	ASSERT_FALSE (error);
	parser.deserialize_keepalive (stream2, header2);
	ASSERT_EQ (1, visitor.keepalive_count);
	ASSERT_NE (parser.status, vxldollar::message_parser::parse_status::success);
}
