#pragma once

#include <vxldollar/lib/errors.hpp>

namespace vxldollar
{
class tomlconfig;
class opencl_config
{
public:
	opencl_config () = default;
	opencl_config (unsigned, unsigned, unsigned);
	vxldollar::error serialize_toml (vxldollar::tomlconfig &) const;
	vxldollar::error deserialize_toml (vxldollar::tomlconfig &);
	unsigned platform{ 0 };
	unsigned device{ 0 };
	unsigned threads{ 1024 * 1024 };
};
}
