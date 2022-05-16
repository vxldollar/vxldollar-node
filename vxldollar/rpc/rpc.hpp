#pragma once

#include <vxldollar/boost/asio/ip/tcp.hpp>
#include <vxldollar/lib/logger_mt.hpp>
#include <vxldollar/lib/rpc_handler_interface.hpp>
#include <vxldollar/lib/rpcconfig.hpp>

namespace boost
{
namespace asio
{
	class io_context;
}
}

namespace vxldollar
{
class rpc_handler_interface;

class rpc
{
public:
	rpc (boost::asio::io_context & io_ctx_a, vxldollar::rpc_config config_a, vxldollar::rpc_handler_interface & rpc_handler_interface_a);
	virtual ~rpc ();
	void start ();
	virtual void accept ();
	void stop ();

	std::uint16_t listening_port ()
	{
		return acceptor.local_endpoint ().port ();
	}

	vxldollar::rpc_config config;
	boost::asio::ip::tcp::acceptor acceptor;
	vxldollar::logger_mt logger;
	boost::asio::io_context & io_ctx;
	vxldollar::rpc_handler_interface & rpc_handler_interface;
	bool stopped{ false };
};

/** Returns the correct RPC implementation based on TLS configuration */
std::unique_ptr<vxldollar::rpc> get_rpc (boost::asio::io_context & io_ctx_a, vxldollar::rpc_config const & config_a, vxldollar::rpc_handler_interface & rpc_handler_interface_a);
}
