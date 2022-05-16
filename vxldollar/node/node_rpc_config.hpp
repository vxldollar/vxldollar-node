#pragma once

#include <vxldollar/lib/rpcconfig.hpp>

#include <boost/property_tree/ptree_fwd.hpp>

#include <string>

namespace boost
{
namespace filesystem
{
	class path;
}
}

namespace vxldollar
{
class tomlconfig;
class rpc_child_process_config final
{
public:
	bool enable{ false };
	std::string rpc_path{ get_default_rpc_filepath () };
};

class node_rpc_config final
{
public:
	vxldollar::error serialize_toml (vxldollar::tomlconfig & toml) const;
	vxldollar::error deserialize_toml (vxldollar::tomlconfig & toml);

	bool enable_sign_hash{ false };
	vxldollar::rpc_child_process_config child_process;

	// Used in tests to ensure requests are modified in specific cases
	void set_request_callback (std::function<void (boost::property_tree::ptree const &)>);
	std::function<void (boost::property_tree::ptree const &)> request_callback;
};
}
