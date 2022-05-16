#pragma once

#include <vxldollar/node/common.hpp>

namespace vxldollar
{
class node;
class system;

namespace transport
{
	class channel;
	class channel_tcp;
}

/** Waits until a TCP connection is established and returns the TCP channel on success*/
std::shared_ptr<vxldollar::transport::channel_tcp> establish_tcp (vxldollar::system &, vxldollar::node &, vxldollar::endpoint const &);
}
