#pragma once

#include <vxldollar/lib/errors.hpp>
#include <vxldollar/lib/numbers.hpp>

#include <string>

namespace vxldollar
{
class tomlconfig;

/** Configuration options for the Qt wallet */
class wallet_config final
{
public:
	wallet_config ();
	/** Update this instance by parsing the given wallet and account */
	vxldollar::error parse (std::string const & wallet_a, std::string const & account_a);
	vxldollar::error serialize_toml (vxldollar::tomlconfig & toml_a) const;
	vxldollar::error deserialize_toml (vxldollar::tomlconfig & toml_a);
	vxldollar::wallet_id wallet;
	vxldollar::account account{};
};
}
