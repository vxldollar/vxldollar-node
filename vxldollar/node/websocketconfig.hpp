#pragma once

#include <vxldollar/lib/config.hpp>
#include <vxldollar/lib/errors.hpp>

#include <memory>

namespace vxldollar
{
class tomlconfig;
class tls_config;
namespace websocket
{
	/** websocket configuration */
	class config final
	{
	public:
		config (vxldollar::network_constants & network_constants);
		vxldollar::error deserialize_toml (vxldollar::tomlconfig & toml_a);
		vxldollar::error serialize_toml (vxldollar::tomlconfig & toml) const;
		vxldollar::network_constants & network_constants;
		bool enabled{ false };
		uint16_t port;
		std::string address;
		/** Optional TLS config */
		std::shared_ptr<vxldollar::tls_config> tls_config;
	};
}
}
