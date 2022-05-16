#include <vxldollar/node/common.hpp>
#include <vxldollar/node/node.hpp>
#include <vxldollar/node/transport/transport.hpp>

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/address_v6.hpp>
#include <boost/format.hpp>

#include <numeric>

namespace
{
class callback_visitor : public vxldollar::message_visitor
{
public:
	void keepalive (vxldollar::keepalive const & message_a) override
	{
		result = vxldollar::stat::detail::keepalive;
	}
	void publish (vxldollar::publish const & message_a) override
	{
		result = vxldollar::stat::detail::publish;
	}
	void confirm_req (vxldollar::confirm_req const & message_a) override
	{
		result = vxldollar::stat::detail::confirm_req;
	}
	void confirm_ack (vxldollar::confirm_ack const & message_a) override
	{
		result = vxldollar::stat::detail::confirm_ack;
	}
	void bulk_pull (vxldollar::bulk_pull const & message_a) override
	{
		result = vxldollar::stat::detail::bulk_pull;
	}
	void bulk_pull_account (vxldollar::bulk_pull_account const & message_a) override
	{
		result = vxldollar::stat::detail::bulk_pull_account;
	}
	void bulk_push (vxldollar::bulk_push const & message_a) override
	{
		result = vxldollar::stat::detail::bulk_push;
	}
	void frontier_req (vxldollar::frontier_req const & message_a) override
	{
		result = vxldollar::stat::detail::frontier_req;
	}
	void node_id_handshake (vxldollar::node_id_handshake const & message_a) override
	{
		result = vxldollar::stat::detail::node_id_handshake;
	}
	void telemetry_req (vxldollar::telemetry_req const & message_a) override
	{
		result = vxldollar::stat::detail::telemetry_req;
	}
	void telemetry_ack (vxldollar::telemetry_ack const & message_a) override
	{
		result = vxldollar::stat::detail::telemetry_ack;
	}
	vxldollar::stat::detail result;
};
}

vxldollar::endpoint vxldollar::transport::map_endpoint_to_v6 (vxldollar::endpoint const & endpoint_a)
{
	auto endpoint_l (endpoint_a);
	if (endpoint_l.address ().is_v4 ())
	{
		endpoint_l = vxldollar::endpoint (boost::asio::ip::address_v6::v4_mapped (endpoint_l.address ().to_v4 ()), endpoint_l.port ());
	}
	return endpoint_l;
}

vxldollar::endpoint vxldollar::transport::map_tcp_to_endpoint (vxldollar::tcp_endpoint const & endpoint_a)
{
	return vxldollar::endpoint (endpoint_a.address (), endpoint_a.port ());
}

vxldollar::tcp_endpoint vxldollar::transport::map_endpoint_to_tcp (vxldollar::endpoint const & endpoint_a)
{
	return vxldollar::tcp_endpoint (endpoint_a.address (), endpoint_a.port ());
}

boost::asio::ip::address vxldollar::transport::map_address_to_subnetwork (boost::asio::ip::address const & address_a)
{
	debug_assert (address_a.is_v6 ());
	static short const ipv6_subnet_prefix_length = 32; // Equivalent to network prefix /32.
	static short const ipv4_subnet_prefix_length = (128 - 32) + 24; // Limits for /24 IPv4 subnetwork
	return address_a.to_v6 ().is_v4_mapped () ? boost::asio::ip::make_network_v6 (address_a.to_v6 (), ipv4_subnet_prefix_length).network () : boost::asio::ip::make_network_v6 (address_a.to_v6 (), ipv6_subnet_prefix_length).network ();
}

boost::asio::ip::address vxldollar::transport::ipv4_address_or_ipv6_subnet (boost::asio::ip::address const & address_a)
{
	debug_assert (address_a.is_v6 ());
	static short const ipv6_address_prefix_length = 48; // /48 IPv6 subnetwork
	return address_a.to_v6 ().is_v4_mapped () ? address_a : boost::asio::ip::make_network_v6 (address_a.to_v6 (), ipv6_address_prefix_length).network ();
}

vxldollar::transport::channel::channel (vxldollar::node & node_a) :
	node (node_a)
{
	set_network_version (node_a.network_params.network.protocol_version);
}

void vxldollar::transport::channel::send (vxldollar::message & message_a, std::function<void (boost::system::error_code const &, std::size_t)> const & callback_a, vxldollar::buffer_drop_policy drop_policy_a)
{
	callback_visitor visitor;
	message_a.visit (visitor);
	auto buffer (message_a.to_shared_const_buffer ());
	auto detail (visitor.result);
	auto is_droppable_by_limiter = drop_policy_a == vxldollar::buffer_drop_policy::limiter;
	auto should_drop (node.network.limiter.should_drop (buffer.size ()));
	if (!is_droppable_by_limiter || !should_drop)
	{
		send_buffer (buffer, callback_a, drop_policy_a);
		node.stats.inc (vxldollar::stat::type::message, detail, vxldollar::stat::dir::out);
	}
	else
	{
		if (callback_a)
		{
			node.background ([callback_a] () {
				callback_a (boost::system::errc::make_error_code (boost::system::errc::not_supported), 0);
			});
		}

		node.stats.inc (vxldollar::stat::type::drop, detail, vxldollar::stat::dir::out);
		if (node.config.logging.network_packet_logging ())
		{
			node.logger.always_log (boost::str (boost::format ("%1% of size %2% dropped") % node.stats.detail_to_string (detail) % buffer.size ()));
		}
	}
}

vxldollar::transport::channel_loopback::channel_loopback (vxldollar::node & node_a) :
	channel (node_a), endpoint (node_a.network.endpoint ())
{
	set_node_id (node_a.node_id.pub);
	set_network_version (node_a.network_params.network.protocol_version);
}

std::size_t vxldollar::transport::channel_loopback::hash_code () const
{
	std::hash<::vxldollar::endpoint> hash;
	return hash (endpoint);
}

bool vxldollar::transport::channel_loopback::operator== (vxldollar::transport::channel const & other_a) const
{
	return endpoint == other_a.get_endpoint ();
}

void vxldollar::transport::channel_loopback::send_buffer (vxldollar::shared_const_buffer const & buffer_a, std::function<void (boost::system::error_code const &, std::size_t)> const & callback_a, vxldollar::buffer_drop_policy drop_policy_a)
{
	release_assert (false && "sending to a loopback channel is not supported");
}

std::string vxldollar::transport::channel_loopback::to_string () const
{
	return boost::str (boost::format ("%1%") % endpoint);
}

boost::asio::ip::address_v6 vxldollar::transport::mapped_from_v4_bytes (unsigned long address_a)
{
	return boost::asio::ip::address_v6::v4_mapped (boost::asio::ip::address_v4 (address_a));
}

boost::asio::ip::address_v6 vxldollar::transport::mapped_from_v4_or_v6 (boost::asio::ip::address const & address_a)
{
	return address_a.is_v4 () ? boost::asio::ip::address_v6::v4_mapped (address_a.to_v4 ()) : address_a.to_v6 ();
}

bool vxldollar::transport::is_ipv4_or_v4_mapped_address (boost::asio::ip::address const & address_a)
{
	return address_a.is_v4 () || address_a.to_v6 ().is_v4_mapped ();
}

bool vxldollar::transport::reserved_address (vxldollar::endpoint const & endpoint_a, bool allow_local_peers)
{
	debug_assert (endpoint_a.address ().is_v6 ());
	auto bytes (endpoint_a.address ().to_v6 ());
	auto result (false);
	static auto const rfc1700_min (mapped_from_v4_bytes (0x00000000ul));
	static auto const rfc1700_max (mapped_from_v4_bytes (0x00fffffful));
	static auto const rfc1918_1_min (mapped_from_v4_bytes (0x0a000000ul));
	static auto const rfc1918_1_max (mapped_from_v4_bytes (0x0afffffful));
	static auto const rfc1918_2_min (mapped_from_v4_bytes (0xac100000ul));
	static auto const rfc1918_2_max (mapped_from_v4_bytes (0xac1ffffful));
	static auto const rfc1918_3_min (mapped_from_v4_bytes (0xc0a80000ul));
	static auto const rfc1918_3_max (mapped_from_v4_bytes (0xc0a8fffful));
	static auto const rfc6598_min (mapped_from_v4_bytes (0x64400000ul));
	static auto const rfc6598_max (mapped_from_v4_bytes (0x647ffffful));
	static auto const rfc5737_1_min (mapped_from_v4_bytes (0xc0000200ul));
	static auto const rfc5737_1_max (mapped_from_v4_bytes (0xc00002fful));
	static auto const rfc5737_2_min (mapped_from_v4_bytes (0xc6336400ul));
	static auto const rfc5737_2_max (mapped_from_v4_bytes (0xc63364fful));
	static auto const rfc5737_3_min (mapped_from_v4_bytes (0xcb007100ul));
	static auto const rfc5737_3_max (mapped_from_v4_bytes (0xcb0071fful));
	static auto const ipv4_multicast_min (mapped_from_v4_bytes (0xe0000000ul));
	static auto const ipv4_multicast_max (mapped_from_v4_bytes (0xeffffffful));
	static auto const rfc6890_min (mapped_from_v4_bytes (0xf0000000ul));
	static auto const rfc6890_max (mapped_from_v4_bytes (0xfffffffful));
	static auto const rfc6666_min (boost::asio::ip::make_address_v6 ("100::"));
	static auto const rfc6666_max (boost::asio::ip::make_address_v6 ("100::ffff:ffff:ffff:ffff"));
	static auto const rfc3849_min (boost::asio::ip::make_address_v6 ("2001:db8::"));
	static auto const rfc3849_max (boost::asio::ip::make_address_v6 ("2001:db8:ffff:ffff:ffff:ffff:ffff:ffff"));
	static auto const rfc4193_min (boost::asio::ip::make_address_v6 ("fc00::"));
	static auto const rfc4193_max (boost::asio::ip::make_address_v6 ("fd00:ffff:ffff:ffff:ffff:ffff:ffff:ffff"));
	static auto const ipv6_multicast_min (boost::asio::ip::make_address_v6 ("ff00::"));
	static auto const ipv6_multicast_max (boost::asio::ip::make_address_v6 ("ff00:ffff:ffff:ffff:ffff:ffff:ffff:ffff"));
	if (endpoint_a.port () == 0)
	{
		result = true;
	}
	else if (bytes >= rfc1700_min && bytes <= rfc1700_max)
	{
		result = true;
	}
	else if (bytes >= rfc5737_1_min && bytes <= rfc5737_1_max)
	{
		result = true;
	}
	else if (bytes >= rfc5737_2_min && bytes <= rfc5737_2_max)
	{
		result = true;
	}
	else if (bytes >= rfc5737_3_min && bytes <= rfc5737_3_max)
	{
		result = true;
	}
	else if (bytes >= ipv4_multicast_min && bytes <= ipv4_multicast_max)
	{
		result = true;
	}
	else if (bytes >= rfc6890_min && bytes <= rfc6890_max)
	{
		result = true;
	}
	else if (bytes >= rfc6666_min && bytes <= rfc6666_max)
	{
		result = true;
	}
	else if (bytes >= rfc3849_min && bytes <= rfc3849_max)
	{
		result = true;
	}
	else if (bytes >= ipv6_multicast_min && bytes <= ipv6_multicast_max)
	{
		result = true;
	}
	else if (!allow_local_peers)
	{
		if (bytes >= rfc1918_1_min && bytes <= rfc1918_1_max)
		{
			result = true;
		}
		else if (bytes >= rfc1918_2_min && bytes <= rfc1918_2_max)
		{
			result = true;
		}
		else if (bytes >= rfc1918_3_min && bytes <= rfc1918_3_max)
		{
			result = true;
		}
		else if (bytes >= rfc6598_min && bytes <= rfc6598_max)
		{
			result = true;
		}
		else if (bytes >= rfc4193_min && bytes <= rfc4193_max)
		{
			result = true;
		}
	}
	return result;
}

using namespace std::chrono_literals;

vxldollar::bandwidth_limiter::bandwidth_limiter (double const limit_burst_ratio_a, std::size_t const limit_a) :
	bucket (static_cast<std::size_t> (limit_a * limit_burst_ratio_a), limit_a)
{
}

bool vxldollar::bandwidth_limiter::should_drop (std::size_t const & message_size_a)
{
	return !bucket.try_consume (vxldollar::narrow_cast<unsigned int> (message_size_a));
}

void vxldollar::bandwidth_limiter::reset (double const limit_burst_ratio_a, std::size_t const limit_a)
{
	bucket.reset (static_cast<std::size_t> (limit_a * limit_burst_ratio_a), limit_a);
}
