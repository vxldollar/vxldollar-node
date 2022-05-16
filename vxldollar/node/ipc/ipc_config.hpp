#pragma once

#include <vxldollar/lib/config.hpp>
#include <vxldollar/lib/errors.hpp>

#include <string>

namespace vxldollar
{
class tomlconfig;
namespace ipc
{
	/** Base class for transport configurations */
	class ipc_config_transport
	{
	public:
		virtual ~ipc_config_transport () = default;
		bool enabled{ false };
		bool allow_unsafe{ false };
		std::size_t io_timeout{ 15 };
		long io_threads{ -1 };
	};

	/**
	 * Flatbuffers encoding config. See TOML serialization calls for details about each field.
	 */
	class ipc_config_flatbuffers final
	{
	public:
		bool skip_unexpected_fields_in_json{ true };
		bool verify_buffers{ true };
	};

	/** Domain socket specific transport config */
	class ipc_config_domain_socket : public ipc_config_transport
	{
	public:
		/**
		 * Default domain socket path for Unix systems. Once Boost supports Windows 10 usocks,
		 * this value will be conditional on OS.
		 */
		std::string path{ "/tmp/vxldollar" };
	};

	/** TCP specific transport config */
	class ipc_config_tcp_socket : public ipc_config_transport
	{
	public:
		ipc_config_tcp_socket (vxldollar::network_constants & network_constants) :
			network_constants{ network_constants },
			port{ network_constants.default_ipc_port }
		{
		}
		vxldollar::network_constants & network_constants;
		/** Listening port */
		uint16_t port;
	};

	/** IPC configuration */
	class ipc_config
	{
	public:
		ipc_config (vxldollar::network_constants & network_constants) :
			transport_tcp{ network_constants }
		{
		}
		vxldollar::error deserialize_toml (vxldollar::tomlconfig & toml_a);
		vxldollar::error serialize_toml (vxldollar::tomlconfig & toml) const;
		ipc_config_domain_socket transport_domain;
		ipc_config_tcp_socket transport_tcp;
		ipc_config_flatbuffers flatbuffers;
	};
}
}
