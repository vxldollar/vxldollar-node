#include <vxldollar/node/common.hpp>
#include <vxldollar/node/testing.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>

namespace vxldollar
{
void force_vxldollar_dev_network ();
}
namespace
{
std::shared_ptr<vxldollar::system> system0;
std::shared_ptr<vxldollar::node> node0;

class fuzz_visitor : public vxldollar::message_visitor
{
public:
	virtual void keepalive (vxldollar::keepalive const &) override
	{
	}
	virtual void publish (vxldollar::publish const &) override
	{
	}
	virtual void confirm_req (vxldollar::confirm_req const &) override
	{
	}
	virtual void confirm_ack (vxldollar::confirm_ack const &) override
	{
	}
	virtual void bulk_pull (vxldollar::bulk_pull const &) override
	{
	}
	virtual void bulk_pull_account (vxldollar::bulk_pull_account const &) override
	{
	}
	virtual void bulk_push (vxldollar::bulk_push const &) override
	{
	}
	virtual void frontier_req (vxldollar::frontier_req const &) override
	{
	}
	virtual void node_id_handshake (vxldollar::node_id_handshake const &) override
	{
	}
	virtual void telemetry_req (vxldollar::telemetry_req const &) override
	{
	}
	virtual void telemetry_ack (vxldollar::telemetry_ack const &) override
	{
	}
};
}

/** Fuzz live message parsing. This covers parsing and block/vote uniquing. */
void fuzz_message_parser (uint8_t const * Data, size_t Size)
{
	static bool initialized = false;
	if (!initialized)
	{
		vxldollar::force_vxldollar_dev_network ();
		initialized = true;
		system0 = std::make_shared<vxldollar::system> (1);
		node0 = system0->nodes[0];
	}

	fuzz_visitor visitor;
	vxldollar::message_parser parser (node0->network.publish_filter, node0->block_uniquer, node0->vote_uniquer, visitor, node0->work);
	parser.deserialize_buffer (Data, Size);
}

/** Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput (uint8_t const * Data, size_t Size)
{
	fuzz_message_parser (Data, Size);
	return 0;
}
