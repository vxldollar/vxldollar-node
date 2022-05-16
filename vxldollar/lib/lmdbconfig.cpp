#include <vxldollar/lib/lmdbconfig.hpp>
#include <vxldollar/lib/tomlconfig.hpp>
#include <vxldollar/secure/common.hpp>

#include <iostream>

vxldollar::error vxldollar::lmdb_config::serialize_toml (vxldollar::tomlconfig & toml) const
{
	std::string sync_string;
	switch (sync)
	{
		case vxldollar::lmdb_config::sync_strategy::always:
			sync_string = "always";
			break;
		case vxldollar::lmdb_config::sync_strategy::nosync_safe:
			sync_string = "nosync_safe";
			break;
		case vxldollar::lmdb_config::sync_strategy::nosync_unsafe:
			sync_string = "nosync_unsafe";
			break;
		case vxldollar::lmdb_config::sync_strategy::nosync_unsafe_large_memory:
			sync_string = "nosync_unsafe_large_memory";
			break;
	}

	toml.put ("sync", sync_string, "Sync strategy for flushing commits to the ledger database. This does not affect the wallet database.\ntype:string,{always, nosync_safe, nosync_unsafe, nosync_unsafe_large_memory}");
	toml.put ("max_databases", max_databases, "Maximum open lmdb databases. Increase default if more than 100 wallets is required.\nNote: external management is recommended when a large amounts of wallets are required (see https://docs.vxldollar.org/integration-guides/key-management/).\ntype:uin32");
	toml.put ("map_size", map_size, "Maximum ledger database map size in bytes.\ntype:uint64");
	return toml.get_error ();
}

vxldollar::error vxldollar::lmdb_config::deserialize_toml (vxldollar::tomlconfig & toml)
{
	auto default_max_databases = max_databases;
	toml.get_optional<uint32_t> ("max_databases", max_databases);
	toml.get_optional<size_t> ("map_size", map_size);

	if (!toml.get_error ())
	{
		std::string sync_string = "always";
		toml.get_optional<std::string> ("sync", sync_string);
		if (sync_string == "always")
		{
			sync = vxldollar::lmdb_config::sync_strategy::always;
		}
		else if (sync_string == "nosync_safe")
		{
			sync = vxldollar::lmdb_config::sync_strategy::nosync_safe;
		}
		else if (sync_string == "nosync_unsafe")
		{
			sync = vxldollar::lmdb_config::sync_strategy::nosync_unsafe;
		}
		else if (sync_string == "nosync_unsafe_large_memory")
		{
			sync = vxldollar::lmdb_config::sync_strategy::nosync_unsafe_large_memory;
		}
		else
		{
			toml.get_error ().set (sync_string + " is not a valid sync option");
		}
	}

	return toml.get_error ();
}
