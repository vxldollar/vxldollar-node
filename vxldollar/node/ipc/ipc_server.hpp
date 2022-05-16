#pragma once

#include <vxldollar/ipc_flatbuffers_lib/generated/flatbuffers/vxldollarapi_generated.h>
#include <vxldollar/lib/errors.hpp>
#include <vxldollar/lib/ipc.hpp>
#include <vxldollar/node/ipc/ipc_access_config.hpp>
#include <vxldollar/node/ipc/ipc_broker.hpp>
#include <vxldollar/node/node_rpc_config.hpp>

#include <atomic>
#include <memory>
#include <mutex>

namespace flatbuffers
{
class Parser;
}
namespace vxldollar
{
class node;
class error;
namespace ipc
{
	class access;
	/** The IPC server accepts connections on one or more configured transports */
	class ipc_server final
	{
	public:
		ipc_server (vxldollar::node & node, vxldollar::node_rpc_config const & node_rpc_config);
		~ipc_server ();
		void stop ();

		std::optional<std::uint16_t> listening_tcp_port () const;

		vxldollar::node & node;
		vxldollar::node_rpc_config const & node_rpc_config;

		/** Unique counter/id shared across sessions */
		std::atomic<uint64_t> id_dispenser{ 1 };
		std::shared_ptr<vxldollar::ipc::broker> get_broker ();
		vxldollar::ipc::access & get_access ();
		vxldollar::error reload_access_config ();

	private:
		void setup_callbacks ();
		std::shared_ptr<vxldollar::ipc::broker> broker;
		vxldollar::ipc::access access;
		std::unique_ptr<dsock_file_remover> file_remover;
		std::vector<std::shared_ptr<vxldollar::ipc::transport>> transports;
	};
}
}
