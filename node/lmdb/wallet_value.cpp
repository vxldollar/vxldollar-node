#include <vxldollar/node/lmdb/wallet_value.hpp>

vxldollar::wallet_value::wallet_value (vxldollar::db_val<MDB_val> const & val_a)
{
	debug_assert (val_a.size () == sizeof (*this));
	std::copy (reinterpret_cast<uint8_t const *> (val_a.data ()), reinterpret_cast<uint8_t const *> (val_a.data ()) + sizeof (key), key.chars.begin ());
	std::copy (reinterpret_cast<uint8_t const *> (val_a.data ()) + sizeof (key), reinterpret_cast<uint8_t const *> (val_a.data ()) + sizeof (key) + sizeof (work), reinterpret_cast<char *> (&work));
}

vxldollar::wallet_value::wallet_value (vxldollar::raw_key const & key_a, uint64_t work_a) :
	key (key_a),
	work (work_a)
{
}

vxldollar::db_val<MDB_val> vxldollar::wallet_value::val () const
{
	static_assert (sizeof (*this) == sizeof (key) + sizeof (work), "Class not packed");
	return vxldollar::db_val<MDB_val> (sizeof (*this), const_cast<vxldollar::wallet_value *> (this));
}
