#pragma once

#include <vxldollar/lib/numbers.hpp>

namespace vxldollar
{
class inactive_cache_status final
{
public:
	bool bootstrap_started{ false };

	/** Did item reach config threshold to start an impromptu election? */
	bool election_started{ false };

	/** Did item reach votes quorum? (minimum config value) */
	bool confirmed{ false };

	/** Last votes tally for block */
	vxldollar::uint128_t tally{ 0 };

	bool operator!= (inactive_cache_status const other) const;

	std::string to_string () const;
};

}
