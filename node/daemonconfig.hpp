#pragma once

#include <vxldollar/lib/errors.hpp>
#include <vxldollar/node/node_pow_server_config.hpp>
#include <vxldollar/node/node_rpc_config.hpp>
#include <vxldollar/node/nodeconfig.hpp>
#include <vxldollar/node/openclconfig.hpp>

#include <vector>

namespace vxldollar
{
class tomlconfig;
class daemon_config
{
public:
	daemon_config () = default;
	daemon_config (boost::filesystem::path const & data_path, vxldollar::network_params & network_params);
	vxldollar::error deserialize_toml (vxldollar::tomlconfig &);
	vxldollar::error serialize_toml (vxldollar::tomlconfig &);
	bool rpc_enable{ false };
	vxldollar::node_rpc_config rpc;
	vxldollar::node_config node;
	bool opencl_enable{ false };
	vxldollar::opencl_config opencl;
	vxldollar::node_pow_server_config pow_server;
	boost::filesystem::path data_path;
};

vxldollar::error read_node_config_toml (boost::filesystem::path const &, vxldollar::daemon_config & config_a, std::vector<std::string> const & config_overrides = std::vector<std::string> ());
}
