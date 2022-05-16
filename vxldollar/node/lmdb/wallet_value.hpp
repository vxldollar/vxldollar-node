#pragma once

#include <vxldollar/lib/numbers.hpp>
#include <vxldollar/secure/store.hpp>

#include <lmdb/libraries/liblmdb/lmdb.h>

namespace vxldollar
{
class wallet_value
{
public:
	wallet_value () = default;
	wallet_value (vxldollar::db_val<MDB_val> const &);
	wallet_value (vxldollar::raw_key const &, uint64_t);
	vxldollar::db_val<MDB_val> val () const;
	vxldollar::raw_key key;
	uint64_t work;
};
}
