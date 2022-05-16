#include <vxldollar/node/node.hpp>
#include <vxldollar/test_common/network.hpp>
#include <vxldollar/test_common/system.hpp>
#include <vxldollar/test_common/testutil.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <future>

using namespace std::chrono_literals;

std::shared_ptr<vxldollar::transport::channel_tcp> vxldollar::establish_tcp (vxldollar::system & system, vxldollar::node & node, vxldollar::endpoint const & endpoint)
{
	debug_assert (node.network.endpoint () != endpoint && "Establishing TCP to self is not allowed");

	std::shared_ptr<vxldollar::transport::channel_tcp> result;
	debug_assert (!node.flags.disable_tcp_realtime);
	node.network.tcp_channels.start_tcp (endpoint);
	auto error = system.poll_until_true (2s, [&result, &node, &endpoint] {
		result = node.network.tcp_channels.find_channel (vxldollar::transport::map_endpoint_to_tcp (endpoint));
		return result != nullptr;
	});
	return result;
}
