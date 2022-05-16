#include <vxldollar/lib/tomlconfig.hpp>
#include <vxldollar/node/node_pow_server_config.hpp>

vxldollar::error vxldollar::node_pow_server_config::serialize_toml (vxldollar::tomlconfig & toml) const
{
	toml.put ("enable", enable, "Value is currently not in use. Enable or disable starting Vxldollar PoW Server as a child process.\ntype:bool");
	toml.put ("vxldollar_pow_server_path", pow_server_path, "Value is currently not in use. Path to the vxldollar_pow_server executable.\ntype:string,path");
	return toml.get_error ();
}

vxldollar::error vxldollar::node_pow_server_config::deserialize_toml (vxldollar::tomlconfig & toml)
{
	toml.get_optional<bool> ("enable", enable);
	toml.get_optional<std::string> ("vxldollar_pow_server_path", pow_server_path);

	return toml.get_error ();
}
