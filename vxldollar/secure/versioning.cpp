#include <vxldollar/secure/versioning.hpp>

#include <boost/endian/conversion.hpp>

#include <lmdb/libraries/liblmdb/lmdb.h>

vxldollar::pending_info_v14::pending_info_v14 (vxldollar::account const & source_a, vxldollar::amount const & amount_a, vxldollar::epoch epoch_a) :
	source (source_a),
	amount (amount_a),
	epoch (epoch_a)
{
}

bool vxldollar::pending_info_v14::deserialize (vxldollar::stream & stream_a)
{
	auto error (false);
	try
	{
		vxldollar::read (stream_a, source.bytes);
		vxldollar::read (stream_a, amount.bytes);
	}
	catch (std::runtime_error const &)
	{
		error = true;
	}

	return error;
}

size_t vxldollar::pending_info_v14::db_size () const
{
	return sizeof (source) + sizeof (amount);
}

bool vxldollar::pending_info_v14::operator== (vxldollar::pending_info_v14 const & other_a) const
{
	return source == other_a.source && amount == other_a.amount && epoch == other_a.epoch;
}

vxldollar::account_info_v14::account_info_v14 (vxldollar::block_hash const & head_a, vxldollar::block_hash const & rep_block_a, vxldollar::block_hash const & open_block_a, vxldollar::amount const & balance_a, uint64_t modified_a, uint64_t block_count_a, uint64_t confirmation_height_a, vxldollar::epoch epoch_a) :
	head (head_a),
	rep_block (rep_block_a),
	open_block (open_block_a),
	balance (balance_a),
	modified (modified_a),
	block_count (block_count_a),
	confirmation_height (confirmation_height_a),
	epoch (epoch_a)
{
}

size_t vxldollar::account_info_v14::db_size () const
{
	debug_assert (reinterpret_cast<uint8_t const *> (this) == reinterpret_cast<uint8_t const *> (&head));
	debug_assert (reinterpret_cast<uint8_t const *> (&head) + sizeof (head) == reinterpret_cast<uint8_t const *> (&rep_block));
	debug_assert (reinterpret_cast<uint8_t const *> (&rep_block) + sizeof (rep_block) == reinterpret_cast<uint8_t const *> (&open_block));
	debug_assert (reinterpret_cast<uint8_t const *> (&open_block) + sizeof (open_block) == reinterpret_cast<uint8_t const *> (&balance));
	debug_assert (reinterpret_cast<uint8_t const *> (&balance) + sizeof (balance) == reinterpret_cast<uint8_t const *> (&modified));
	debug_assert (reinterpret_cast<uint8_t const *> (&modified) + sizeof (modified) == reinterpret_cast<uint8_t const *> (&block_count));
	debug_assert (reinterpret_cast<uint8_t const *> (&block_count) + sizeof (block_count) == reinterpret_cast<uint8_t const *> (&confirmation_height));
	return sizeof (head) + sizeof (rep_block) + sizeof (open_block) + sizeof (balance) + sizeof (modified) + sizeof (block_count) + sizeof (confirmation_height);
}

vxldollar::block_sideband_v14::block_sideband_v14 (vxldollar::block_type type_a, vxldollar::account const & account_a, vxldollar::block_hash const & successor_a, vxldollar::amount const & balance_a, uint64_t height_a, uint64_t timestamp_a) :
	type (type_a),
	successor (successor_a),
	account (account_a),
	balance (balance_a),
	height (height_a),
	timestamp (timestamp_a)
{
}

size_t vxldollar::block_sideband_v14::size (vxldollar::block_type type_a)
{
	size_t result (0);
	result += sizeof (successor);
	if (type_a != vxldollar::block_type::state && type_a != vxldollar::block_type::open)
	{
		result += sizeof (account);
	}
	if (type_a != vxldollar::block_type::open)
	{
		result += sizeof (height);
	}
	if (type_a == vxldollar::block_type::receive || type_a == vxldollar::block_type::change || type_a == vxldollar::block_type::open)
	{
		result += sizeof (balance);
	}
	result += sizeof (timestamp);
	return result;
}

void vxldollar::block_sideband_v14::serialize (vxldollar::stream & stream_a) const
{
	vxldollar::write (stream_a, successor.bytes);
	if (type != vxldollar::block_type::state && type != vxldollar::block_type::open)
	{
		vxldollar::write (stream_a, account.bytes);
	}
	if (type != vxldollar::block_type::open)
	{
		vxldollar::write (stream_a, boost::endian::native_to_big (height));
	}
	if (type == vxldollar::block_type::receive || type == vxldollar::block_type::change || type == vxldollar::block_type::open)
	{
		vxldollar::write (stream_a, balance.bytes);
	}
	vxldollar::write (stream_a, boost::endian::native_to_big (timestamp));
}

bool vxldollar::block_sideband_v14::deserialize (vxldollar::stream & stream_a)
{
	bool result (false);
	try
	{
		vxldollar::read (stream_a, successor.bytes);
		if (type != vxldollar::block_type::state && type != vxldollar::block_type::open)
		{
			vxldollar::read (stream_a, account.bytes);
		}
		if (type != vxldollar::block_type::open)
		{
			vxldollar::read (stream_a, height);
			boost::endian::big_to_native_inplace (height);
		}
		else
		{
			height = 1;
		}
		if (type == vxldollar::block_type::receive || type == vxldollar::block_type::change || type == vxldollar::block_type::open)
		{
			vxldollar::read (stream_a, balance.bytes);
		}
		vxldollar::read (stream_a, timestamp);
		boost::endian::big_to_native_inplace (timestamp);
	}
	catch (std::runtime_error &)
	{
		result = true;
	}

	return result;
}

vxldollar::block_sideband_v18::block_sideband_v18 (vxldollar::account const & account_a, vxldollar::block_hash const & successor_a, vxldollar::amount const & balance_a, uint64_t height_a, uint64_t timestamp_a, vxldollar::block_details const & details_a) :
	successor (successor_a),
	account (account_a),
	balance (balance_a),
	height (height_a),
	timestamp (timestamp_a),
	details (details_a)
{
}

vxldollar::block_sideband_v18::block_sideband_v18 (vxldollar::account const & account_a, vxldollar::block_hash const & successor_a, vxldollar::amount const & balance_a, uint64_t height_a, uint64_t timestamp_a, vxldollar::epoch epoch_a, bool is_send, bool is_receive, bool is_epoch) :
	successor (successor_a),
	account (account_a),
	balance (balance_a),
	height (height_a),
	timestamp (timestamp_a),
	details (epoch_a, is_send, is_receive, is_epoch)
{
}

size_t vxldollar::block_sideband_v18::size (vxldollar::block_type type_a)
{
	size_t result (0);
	result += sizeof (successor);
	if (type_a != vxldollar::block_type::state && type_a != vxldollar::block_type::open)
	{
		result += sizeof (account);
	}
	if (type_a != vxldollar::block_type::open)
	{
		result += sizeof (height);
	}
	if (type_a == vxldollar::block_type::receive || type_a == vxldollar::block_type::change || type_a == vxldollar::block_type::open)
	{
		result += sizeof (balance);
	}
	result += sizeof (timestamp);
	if (type_a == vxldollar::block_type::state)
	{
		static_assert (sizeof (vxldollar::epoch) == vxldollar::block_details::size (), "block_details_v18 is larger than the epoch enum");
		result += vxldollar::block_details::size ();
	}
	return result;
}

void vxldollar::block_sideband_v18::serialize (vxldollar::stream & stream_a, vxldollar::block_type type_a) const
{
	vxldollar::write (stream_a, successor.bytes);
	if (type_a != vxldollar::block_type::state && type_a != vxldollar::block_type::open)
	{
		vxldollar::write (stream_a, account.bytes);
	}
	if (type_a != vxldollar::block_type::open)
	{
		vxldollar::write (stream_a, boost::endian::native_to_big (height));
	}
	if (type_a == vxldollar::block_type::receive || type_a == vxldollar::block_type::change || type_a == vxldollar::block_type::open)
	{
		vxldollar::write (stream_a, balance.bytes);
	}
	vxldollar::write (stream_a, boost::endian::native_to_big (timestamp));
	if (type_a == vxldollar::block_type::state)
	{
		details.serialize (stream_a);
	}
}

bool vxldollar::block_sideband_v18::deserialize (vxldollar::stream & stream_a, vxldollar::block_type type_a)
{
	bool result (false);
	try
	{
		vxldollar::read (stream_a, successor.bytes);
		if (type_a != vxldollar::block_type::state && type_a != vxldollar::block_type::open)
		{
			vxldollar::read (stream_a, account.bytes);
		}
		if (type_a != vxldollar::block_type::open)
		{
			vxldollar::read (stream_a, height);
			boost::endian::big_to_native_inplace (height);
		}
		else
		{
			height = 1;
		}
		if (type_a == vxldollar::block_type::receive || type_a == vxldollar::block_type::change || type_a == vxldollar::block_type::open)
		{
			vxldollar::read (stream_a, balance.bytes);
		}
		vxldollar::read (stream_a, timestamp);
		boost::endian::big_to_native_inplace (timestamp);
		if (type_a == vxldollar::block_type::state)
		{
			result = details.deserialize (stream_a);
		}
	}
	catch (std::runtime_error &)
	{
		result = true;
	}

	return result;
}
