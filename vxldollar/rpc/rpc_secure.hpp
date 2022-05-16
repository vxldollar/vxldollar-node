#pragma once
#include <vxldollar/rpc/rpc.hpp>

namespace boost
{
namespace asio
{
	class io_context;
}
}

namespace vxldollar
{
/**
 * Specialization of vxldollar::rpc with TLS support
 */
class rpc_secure : public rpc
{
public:
	rpc_secure (boost::asio::io_context & context_a, vxldollar::rpc_config const & config_a, vxldollar::rpc_handler_interface & rpc_handler_interface_a);

	/** Starts accepting connections */
	void accept () override;
};
}
