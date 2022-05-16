#pragma once

#include <vxldollar/rpc/rpc_connection.hpp>

#include <boost/asio/ssl/stream.hpp>

namespace vxldollar
{
/**
 * Specialization of vxldollar::rpc_connection for establishing TLS connections.
 * Handshakes with client certificates are supported.
 */
class rpc_connection_secure : public rpc_connection
{
public:
	rpc_connection_secure (vxldollar::rpc_config const & rpc_config, boost::asio::io_context & io_ctx, vxldollar::logger_mt & logger, vxldollar::rpc_handler_interface & rpc_handler_interface_a, boost::asio::ssl::context & ssl_context);
	void parse_connection () override;
	void write_completion_handler (std::shared_ptr<vxldollar::rpc_connection> const & rpc) override;
	/** The TLS handshake callback */
	void handle_handshake (boost::system::error_code const & error);
	/** The TLS async shutdown callback */
	void on_shutdown (boost::system::error_code const & error);

private:
	boost::asio::ssl::stream<socket_type &> stream;
};
}
