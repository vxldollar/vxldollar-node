#pragma once

#include <vxldollar/lib/locks.hpp>
#include <vxldollar/lib/rate_limiting.hpp>
#include <vxldollar/lib/stats.hpp>
#include <vxldollar/node/common.hpp>
#include <vxldollar/node/socket.hpp>

#include <boost/asio/ip/network_v6.hpp>

namespace vxldollar
{
class bandwidth_limiter final
{
public:
	// initialize with limit 0 = unbounded
	bandwidth_limiter (double, std::size_t);
	bool should_drop (std::size_t const &);
	void reset (double, std::size_t);

private:
	vxldollar::rate::token_bucket bucket;
};

namespace transport
{
	vxldollar::endpoint map_endpoint_to_v6 (vxldollar::endpoint const &);
	vxldollar::endpoint map_tcp_to_endpoint (vxldollar::tcp_endpoint const &);
	vxldollar::tcp_endpoint map_endpoint_to_tcp (vxldollar::endpoint const &);
	boost::asio::ip::address map_address_to_subnetwork (boost::asio::ip::address const &);
	boost::asio::ip::address ipv4_address_or_ipv6_subnet (boost::asio::ip::address const &);
	boost::asio::ip::address_v6 mapped_from_v4_bytes (unsigned long);
	boost::asio::ip::address_v6 mapped_from_v4_or_v6 (boost::asio::ip::address const &);
	bool is_ipv4_or_v4_mapped_address (boost::asio::ip::address const &);

	// Unassigned, reserved, self
	bool reserved_address (vxldollar::endpoint const &, bool = false);
	static std::chrono::seconds constexpr syn_cookie_cutoff = std::chrono::seconds (5);
	enum class transport_type : uint8_t
	{
		undefined = 0,
		udp = 1,
		tcp = 2,
		loopback = 3
	};
	class channel
	{
	public:
		explicit channel (vxldollar::node &);
		virtual ~channel () = default;
		virtual std::size_t hash_code () const = 0;
		virtual bool operator== (vxldollar::transport::channel const &) const = 0;
		void send (vxldollar::message & message_a, std::function<void (boost::system::error_code const &, std::size_t)> const & callback_a = nullptr, vxldollar::buffer_drop_policy policy_a = vxldollar::buffer_drop_policy::limiter);
		// TODO: investigate clang-tidy warning about default parameters on virtual/override functions
		//
		virtual void send_buffer (vxldollar::shared_const_buffer const &, std::function<void (boost::system::error_code const &, std::size_t)> const & = nullptr, vxldollar::buffer_drop_policy = vxldollar::buffer_drop_policy::limiter) = 0;
		virtual std::string to_string () const = 0;
		virtual vxldollar::endpoint get_endpoint () const = 0;
		virtual vxldollar::tcp_endpoint get_tcp_endpoint () const = 0;
		virtual vxldollar::transport::transport_type get_type () const = 0;

		std::chrono::steady_clock::time_point get_last_bootstrap_attempt () const
		{
			vxldollar::lock_guard<vxldollar::mutex> lk (channel_mutex);
			return last_bootstrap_attempt;
		}

		void set_last_bootstrap_attempt (std::chrono::steady_clock::time_point const time_a)
		{
			vxldollar::lock_guard<vxldollar::mutex> lk (channel_mutex);
			last_bootstrap_attempt = time_a;
		}

		std::chrono::steady_clock::time_point get_last_packet_received () const
		{
			vxldollar::lock_guard<vxldollar::mutex> lk (channel_mutex);
			return last_packet_received;
		}

		void set_last_packet_received (std::chrono::steady_clock::time_point const time_a)
		{
			vxldollar::lock_guard<vxldollar::mutex> lk (channel_mutex);
			last_packet_received = time_a;
		}

		std::chrono::steady_clock::time_point get_last_packet_sent () const
		{
			vxldollar::lock_guard<vxldollar::mutex> lk (channel_mutex);
			return last_packet_sent;
		}

		void set_last_packet_sent (std::chrono::steady_clock::time_point const time_a)
		{
			vxldollar::lock_guard<vxldollar::mutex> lk (channel_mutex);
			last_packet_sent = time_a;
		}

		boost::optional<vxldollar::account> get_node_id_optional () const
		{
			vxldollar::lock_guard<vxldollar::mutex> lk (channel_mutex);
			return node_id;
		}

		vxldollar::account get_node_id () const
		{
			vxldollar::lock_guard<vxldollar::mutex> lk (channel_mutex);
			if (node_id.is_initialized ())
			{
				return node_id.get ();
			}
			else
			{
				return 0;
			}
		}

		void set_node_id (vxldollar::account node_id_a)
		{
			vxldollar::lock_guard<vxldollar::mutex> lk (channel_mutex);
			node_id = node_id_a;
		}

		uint8_t get_network_version () const
		{
			return network_version;
		}

		void set_network_version (uint8_t network_version_a)
		{
			network_version = network_version_a;
		}

		mutable vxldollar::mutex channel_mutex;

	private:
		std::chrono::steady_clock::time_point last_bootstrap_attempt{ std::chrono::steady_clock::time_point () };
		std::chrono::steady_clock::time_point last_packet_received{ std::chrono::steady_clock::now () };
		std::chrono::steady_clock::time_point last_packet_sent{ std::chrono::steady_clock::now () };
		boost::optional<vxldollar::account> node_id{ boost::none };
		std::atomic<uint8_t> network_version{ 0 };

	protected:
		vxldollar::node & node;
	};

	class channel_loopback final : public vxldollar::transport::channel
	{
	public:
		explicit channel_loopback (vxldollar::node &);
		std::size_t hash_code () const override;
		bool operator== (vxldollar::transport::channel const &) const override;
		// TODO: investigate clang-tidy warning about default parameters on virtual/override functions
		//
		void send_buffer (vxldollar::shared_const_buffer const &, std::function<void (boost::system::error_code const &, std::size_t)> const & = nullptr, vxldollar::buffer_drop_policy = vxldollar::buffer_drop_policy::limiter) override;
		std::string to_string () const override;
		bool operator== (vxldollar::transport::channel_loopback const & other_a) const
		{
			return endpoint == other_a.get_endpoint ();
		}

		vxldollar::endpoint get_endpoint () const override
		{
			return endpoint;
		}

		vxldollar::tcp_endpoint get_tcp_endpoint () const override
		{
			return vxldollar::transport::map_endpoint_to_tcp (endpoint);
		}

		vxldollar::transport::transport_type get_type () const override
		{
			return vxldollar::transport::transport_type::loopback;
		}

	private:
		vxldollar::endpoint const endpoint;
	};
} // namespace transport
} // namespace vxldollar

namespace std
{
template <>
struct hash<::vxldollar::transport::channel>
{
	std::size_t operator() (::vxldollar::transport::channel const & channel_a) const
	{
		return channel_a.hash_code ();
	}
};
template <>
struct equal_to<std::reference_wrapper<::vxldollar::transport::channel const>>
{
	bool operator() (std::reference_wrapper<::vxldollar::transport::channel const> const & lhs, std::reference_wrapper<::vxldollar::transport::channel const> const & rhs) const
	{
		return lhs.get () == rhs.get ();
	}
};
}

namespace boost
{
template <>
struct hash<::vxldollar::transport::channel>
{
	std::size_t operator() (::vxldollar::transport::channel const & channel_a) const
	{
		std::hash<::vxldollar::transport::channel> hash;
		return hash (channel_a);
	}
};
template <>
struct hash<std::reference_wrapper<::vxldollar::transport::channel const>>
{
	std::size_t operator() (std::reference_wrapper<::vxldollar::transport::channel const> const & channel_a) const
	{
		std::hash<::vxldollar::transport::channel> hash;
		return hash (channel_a.get ());
	}
};
}
