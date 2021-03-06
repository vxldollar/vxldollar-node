#pragma once

#include <vxldollar/lib/errors.hpp>

#include <thread>

namespace vxldollar
{
class tomlconfig;

/** Configuration options for RocksDB */
class rocksdb_config final
{
public:
	rocksdb_config () :
		enable{ using_rocksdb_in_tests () }
	{
	}
	vxldollar::error serialize_toml (vxldollar::tomlconfig & toml_a) const;
	vxldollar::error deserialize_toml (vxldollar::tomlconfig & toml_a);

	/** To use RocksDB in tests make sure the environment variable TEST_USE_ROCKSDB=1 is set */
	static bool using_rocksdb_in_tests ();

	bool enable{ false };
	uint8_t memory_multiplier{ 2 };
	unsigned io_threads{ std::thread::hardware_concurrency () };
};
}
