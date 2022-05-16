#include <vxldollar/boost/asio/bind_executor.hpp>
#include <vxldollar/lib/tlsconfig.hpp>
#include <vxldollar/rpc/rpc_connection_secure.hpp>
#include <vxldollar/rpc/rpc_secure.hpp>

#include <boost/format.hpp>
#include <boost/polymorphic_pointer_cast.hpp>

#include <iostream>

vxldollar::rpc_secure::rpc_secure (boost::asio::io_context & context_a, vxldollar::rpc_config const & config_a, vxldollar::rpc_handler_interface & rpc_handler_interface_a) :
	rpc (context_a, config_a, rpc_handler_interface_a)
{
}

void vxldollar::rpc_secure::accept ()
{
	auto connection (std::make_shared<vxldollar::rpc_connection_secure> (config, io_ctx, logger, rpc_handler_interface, config.tls_config->ssl_context));
	acceptor.async_accept (connection->socket, boost::asio::bind_executor (connection->strand, [this, connection] (boost::system::error_code const & ec) {
		if (ec != boost::asio::error::operation_aborted && acceptor.is_open ())
		{
			accept ();
		}
		if (!ec)
		{
			connection->parse_connection ();
		}
		else
		{
			logger.always_log (boost::str (boost::format ("Error accepting RPC connections: %1%") % ec));
		}
	}));
}
