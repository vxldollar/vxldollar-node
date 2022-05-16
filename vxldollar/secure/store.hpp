#pragma once

#include <vxldollar/crypto_lib/random_pool.hpp>
#include <vxldollar/lib/diagnosticsconfig.hpp>
#include <vxldollar/lib/lmdbconfig.hpp>
#include <vxldollar/lib/logger_mt.hpp>
#include <vxldollar/lib/memory.hpp>
#include <vxldollar/lib/rocksdbconfig.hpp>
#include <vxldollar/secure/buffer.hpp>
#include <vxldollar/secure/common.hpp>
#include <vxldollar/secure/versioning.hpp>

#include <boost/endian/conversion.hpp>
#include <boost/polymorphic_cast.hpp>

#include <stack>

namespace vxldollar
{
// Move to versioning with a specific version if required for a future upgrade
template <typename T>
class block_w_sideband_v18
{
public:
	std::shared_ptr<T> block;
	vxldollar::block_sideband_v18 sideband;
};

class block_w_sideband
{
public:
	std::shared_ptr<vxldollar::block> block;
	vxldollar::block_sideband sideband;
};

/**
 * Encapsulates database specific container
 */
template <typename Val>
class db_val
{
public:
	db_val (Val const & value_a) :
		value (value_a)
	{
	}

	db_val () :
		db_val (0, nullptr)
	{
	}

	db_val (std::nullptr_t) :
		db_val (0, this)
	{
	}

	db_val (vxldollar::uint128_union const & val_a) :
		db_val (sizeof (val_a), const_cast<vxldollar::uint128_union *> (&val_a))
	{
	}

	db_val (vxldollar::uint256_union const & val_a) :
		db_val (sizeof (val_a), const_cast<vxldollar::uint256_union *> (&val_a))
	{
	}

	db_val (vxldollar::uint512_union const & val_a) :
		db_val (sizeof (val_a), const_cast<vxldollar::uint512_union *> (&val_a))
	{
	}

	db_val (vxldollar::qualified_root const & val_a) :
		db_val (sizeof (val_a), const_cast<vxldollar::qualified_root *> (&val_a))
	{
	}

	db_val (vxldollar::account_info const & val_a) :
		db_val (val_a.db_size (), const_cast<vxldollar::account_info *> (&val_a))
	{
	}

	db_val (vxldollar::account_info_v14 const & val_a) :
		db_val (val_a.db_size (), const_cast<vxldollar::account_info_v14 *> (&val_a))
	{
	}

	db_val (vxldollar::pending_info const & val_a) :
		db_val (val_a.db_size (), const_cast<vxldollar::pending_info *> (&val_a))
	{
		static_assert (std::is_standard_layout<vxldollar::pending_info>::value, "Standard layout is required");
	}

	db_val (vxldollar::pending_info_v14 const & val_a) :
		db_val (val_a.db_size (), const_cast<vxldollar::pending_info_v14 *> (&val_a))
	{
		static_assert (std::is_standard_layout<vxldollar::pending_info_v14>::value, "Standard layout is required");
	}

	db_val (vxldollar::pending_key const & val_a) :
		db_val (sizeof (val_a), const_cast<vxldollar::pending_key *> (&val_a))
	{
		static_assert (std::is_standard_layout<vxldollar::pending_key>::value, "Standard layout is required");
	}

	db_val (vxldollar::unchecked_info const & val_a) :
		buffer (std::make_shared<std::vector<uint8_t>> ())
	{
		{
			vxldollar::vectorstream stream (*buffer);
			val_a.serialize (stream);
		}
		convert_buffer_to_value ();
	}

	db_val (vxldollar::unchecked_key const & val_a) :
		db_val (sizeof (val_a), const_cast<vxldollar::unchecked_key *> (&val_a))
	{
		static_assert (std::is_standard_layout<vxldollar::unchecked_key>::value, "Standard layout is required");
	}

	db_val (vxldollar::confirmation_height_info const & val_a) :
		buffer (std::make_shared<std::vector<uint8_t>> ())
	{
		{
			vxldollar::vectorstream stream (*buffer);
			val_a.serialize (stream);
		}
		convert_buffer_to_value ();
	}

	db_val (vxldollar::block_info const & val_a) :
		db_val (sizeof (val_a), const_cast<vxldollar::block_info *> (&val_a))
	{
		static_assert (std::is_standard_layout<vxldollar::block_info>::value, "Standard layout is required");
	}

	db_val (vxldollar::endpoint_key const & val_a) :
		db_val (sizeof (val_a), const_cast<vxldollar::endpoint_key *> (&val_a))
	{
		static_assert (std::is_standard_layout<vxldollar::endpoint_key>::value, "Standard layout is required");
	}

	db_val (std::shared_ptr<vxldollar::block> const & val_a) :
		buffer (std::make_shared<std::vector<uint8_t>> ())
	{
		{
			vxldollar::vectorstream stream (*buffer);
			vxldollar::serialize_block (stream, *val_a);
		}
		convert_buffer_to_value ();
	}

	db_val (uint64_t val_a) :
		buffer (std::make_shared<std::vector<uint8_t>> ())
	{
		{
			boost::endian::native_to_big_inplace (val_a);
			vxldollar::vectorstream stream (*buffer);
			vxldollar::write (stream, val_a);
		}
		convert_buffer_to_value ();
	}

	explicit operator vxldollar::account_info () const
	{
		vxldollar::account_info result;
		debug_assert (size () == result.db_size ());
		std::copy (reinterpret_cast<uint8_t const *> (data ()), reinterpret_cast<uint8_t const *> (data ()) + result.db_size (), reinterpret_cast<uint8_t *> (&result));
		return result;
	}

	explicit operator vxldollar::account_info_v14 () const
	{
		vxldollar::account_info_v14 result;
		debug_assert (size () == result.db_size ());
		std::copy (reinterpret_cast<uint8_t const *> (data ()), reinterpret_cast<uint8_t const *> (data ()) + result.db_size (), reinterpret_cast<uint8_t *> (&result));
		return result;
	}

	explicit operator vxldollar::block_info () const
	{
		vxldollar::block_info result;
		debug_assert (size () == sizeof (result));
		static_assert (sizeof (vxldollar::block_info::account) + sizeof (vxldollar::block_info::balance) == sizeof (result), "Packed class");
		std::copy (reinterpret_cast<uint8_t const *> (data ()), reinterpret_cast<uint8_t const *> (data ()) + sizeof (result), reinterpret_cast<uint8_t *> (&result));
		return result;
	}

	explicit operator vxldollar::pending_info_v14 () const
	{
		vxldollar::pending_info_v14 result;
		debug_assert (size () == result.db_size ());
		std::copy (reinterpret_cast<uint8_t const *> (data ()), reinterpret_cast<uint8_t const *> (data ()) + result.db_size (), reinterpret_cast<uint8_t *> (&result));
		return result;
	}

	explicit operator vxldollar::pending_info () const
	{
		vxldollar::pending_info result;
		debug_assert (size () == result.db_size ());
		std::copy (reinterpret_cast<uint8_t const *> (data ()), reinterpret_cast<uint8_t const *> (data ()) + result.db_size (), reinterpret_cast<uint8_t *> (&result));
		return result;
	}

	explicit operator vxldollar::pending_key () const
	{
		vxldollar::pending_key result;
		debug_assert (size () == sizeof (result));
		static_assert (sizeof (vxldollar::pending_key::account) + sizeof (vxldollar::pending_key::hash) == sizeof (result), "Packed class");
		std::copy (reinterpret_cast<uint8_t const *> (data ()), reinterpret_cast<uint8_t const *> (data ()) + sizeof (result), reinterpret_cast<uint8_t *> (&result));
		return result;
	}

	explicit operator vxldollar::confirmation_height_info () const
	{
		vxldollar::bufferstream stream (reinterpret_cast<uint8_t const *> (data ()), size ());
		vxldollar::confirmation_height_info result;
		bool error (result.deserialize (stream));
		(void)error;
		debug_assert (!error);
		return result;
	}

	explicit operator vxldollar::unchecked_info () const
	{
		vxldollar::bufferstream stream (reinterpret_cast<uint8_t const *> (data ()), size ());
		vxldollar::unchecked_info result;
		bool error (result.deserialize (stream));
		(void)error;
		debug_assert (!error);
		return result;
	}

	explicit operator vxldollar::unchecked_key () const
	{
		vxldollar::unchecked_key result;
		debug_assert (size () == sizeof (result));
		static_assert (sizeof (vxldollar::unchecked_key::previous) + sizeof (vxldollar::pending_key::hash) == sizeof (result), "Packed class");
		std::copy (reinterpret_cast<uint8_t const *> (data ()), reinterpret_cast<uint8_t const *> (data ()) + sizeof (result), reinterpret_cast<uint8_t *> (&result));
		return result;
	}

	explicit operator vxldollar::uint128_union () const
	{
		return convert<vxldollar::uint128_union> ();
	}

	explicit operator vxldollar::amount () const
	{
		return convert<vxldollar::amount> ();
	}

	explicit operator vxldollar::block_hash () const
	{
		return convert<vxldollar::block_hash> ();
	}

	explicit operator vxldollar::public_key () const
	{
		return convert<vxldollar::public_key> ();
	}

	explicit operator vxldollar::qualified_root () const
	{
		return convert<vxldollar::qualified_root> ();
	}

	explicit operator vxldollar::uint256_union () const
	{
		return convert<vxldollar::uint256_union> ();
	}

	explicit operator vxldollar::uint512_union () const
	{
		return convert<vxldollar::uint512_union> ();
	}

	explicit operator std::array<char, 64> () const
	{
		vxldollar::bufferstream stream (reinterpret_cast<uint8_t const *> (data ()), size ());
		std::array<char, 64> result;
		auto error = vxldollar::try_read (stream, result);
		(void)error;
		debug_assert (!error);
		return result;
	}

	explicit operator vxldollar::endpoint_key () const
	{
		vxldollar::endpoint_key result;
		std::copy (reinterpret_cast<uint8_t const *> (data ()), reinterpret_cast<uint8_t const *> (data ()) + sizeof (result), reinterpret_cast<uint8_t *> (&result));
		return result;
	}

	template <class Block>
	explicit operator block_w_sideband_v18<Block> () const
	{
		vxldollar::bufferstream stream (reinterpret_cast<uint8_t const *> (data ()), size ());
		auto error (false);
		block_w_sideband_v18<Block> block_w_sideband;
		block_w_sideband.block = std::make_shared<Block> (error, stream);
		release_assert (!error);

		error = block_w_sideband.sideband.deserialize (stream, block_w_sideband.block->type ());
		release_assert (!error);

		return block_w_sideband;
	}

	explicit operator block_w_sideband () const
	{
		vxldollar::bufferstream stream (reinterpret_cast<uint8_t const *> (data ()), size ());
		vxldollar::block_w_sideband block_w_sideband;
		block_w_sideband.block = (vxldollar::deserialize_block (stream));
		auto error = block_w_sideband.sideband.deserialize (stream, block_w_sideband.block->type ());
		release_assert (!error);
		block_w_sideband.block->sideband_set (block_w_sideband.sideband);
		return block_w_sideband;
	}

	explicit operator state_block_w_sideband_v14 () const
	{
		vxldollar::bufferstream stream (reinterpret_cast<uint8_t const *> (data ()), size ());
		auto error (false);
		vxldollar::state_block_w_sideband_v14 block_w_sideband;
		block_w_sideband.state_block = std::make_shared<vxldollar::state_block> (error, stream);
		debug_assert (!error);

		block_w_sideband.sideband.type = vxldollar::block_type::state;
		error = block_w_sideband.sideband.deserialize (stream);
		debug_assert (!error);

		return block_w_sideband;
	}

	explicit operator std::nullptr_t () const
	{
		return nullptr;
	}

	explicit operator vxldollar::no_value () const
	{
		return no_value::dummy;
	}

	explicit operator std::shared_ptr<vxldollar::block> () const
	{
		vxldollar::bufferstream stream (reinterpret_cast<uint8_t const *> (data ()), size ());
		std::shared_ptr<vxldollar::block> result (vxldollar::deserialize_block (stream));
		return result;
	}

	template <typename Block>
	std::shared_ptr<Block> convert_to_block () const
	{
		vxldollar::bufferstream stream (reinterpret_cast<uint8_t const *> (data ()), size ());
		auto error (false);
		auto result (std::make_shared<Block> (error, stream));
		debug_assert (!error);
		return result;
	}

	explicit operator std::shared_ptr<vxldollar::send_block> () const
	{
		return convert_to_block<vxldollar::send_block> ();
	}

	explicit operator std::shared_ptr<vxldollar::receive_block> () const
	{
		return convert_to_block<vxldollar::receive_block> ();
	}

	explicit operator std::shared_ptr<vxldollar::open_block> () const
	{
		return convert_to_block<vxldollar::open_block> ();
	}

	explicit operator std::shared_ptr<vxldollar::change_block> () const
	{
		return convert_to_block<vxldollar::change_block> ();
	}

	explicit operator std::shared_ptr<vxldollar::state_block> () const
	{
		return convert_to_block<vxldollar::state_block> ();
	}

	explicit operator std::shared_ptr<vxldollar::vote> () const
	{
		vxldollar::bufferstream stream (reinterpret_cast<uint8_t const *> (data ()), size ());
		auto error (false);
		auto result (vxldollar::make_shared<vxldollar::vote> (error, stream));
		debug_assert (!error);
		return result;
	}

	explicit operator uint64_t () const
	{
		uint64_t result;
		vxldollar::bufferstream stream (reinterpret_cast<uint8_t const *> (data ()), size ());
		auto error (vxldollar::try_read (stream, result));
		(void)error;
		debug_assert (!error);
		boost::endian::big_to_native_inplace (result);
		return result;
	}

	operator Val * () const
	{
		// Allow passing a temporary to a non-c++ function which doesn't have constness
		return const_cast<Val *> (&value);
	}

	operator Val const & () const
	{
		return value;
	}

	// Must be specialized
	void * data () const;
	size_t size () const;
	db_val (size_t size_a, void * data_a);
	void convert_buffer_to_value ();

	Val value;
	std::shared_ptr<std::vector<uint8_t>> buffer;

private:
	template <typename T>
	T convert () const
	{
		T result;
		debug_assert (size () == sizeof (result));
		std::copy (reinterpret_cast<uint8_t const *> (data ()), reinterpret_cast<uint8_t const *> (data ()) + sizeof (result), result.bytes.data ());
		return result;
	}
};

class transaction;
class store;

/**
 * Determine the representative for this block
 */
class representative_visitor final : public vxldollar::block_visitor
{
public:
	representative_visitor (vxldollar::transaction const & transaction_a, vxldollar::store & store_a);
	~representative_visitor () = default;
	void compute (vxldollar::block_hash const & hash_a);
	void send_block (vxldollar::send_block const & block_a) override;
	void receive_block (vxldollar::receive_block const & block_a) override;
	void open_block (vxldollar::open_block const & block_a) override;
	void change_block (vxldollar::change_block const & block_a) override;
	void state_block (vxldollar::state_block const & block_a) override;
	vxldollar::transaction const & transaction;
	vxldollar::store & store;
	vxldollar::block_hash current;
	vxldollar::block_hash result;
};
template <typename T, typename U>
class store_iterator_impl
{
public:
	virtual ~store_iterator_impl () = default;
	virtual vxldollar::store_iterator_impl<T, U> & operator++ () = 0;
	virtual vxldollar::store_iterator_impl<T, U> & operator-- () = 0;
	virtual bool operator== (vxldollar::store_iterator_impl<T, U> const & other_a) const = 0;
	virtual bool is_end_sentinal () const = 0;
	virtual void fill (std::pair<T, U> &) const = 0;
	vxldollar::store_iterator_impl<T, U> & operator= (vxldollar::store_iterator_impl<T, U> const &) = delete;
	bool operator== (vxldollar::store_iterator_impl<T, U> const * other_a) const
	{
		return (other_a != nullptr && *this == *other_a) || (other_a == nullptr && is_end_sentinal ());
	}
	bool operator!= (vxldollar::store_iterator_impl<T, U> const & other_a) const
	{
		return !(*this == other_a);
	}
};
/**
 * Iterates the key/value pairs of a transaction
 */
template <typename T, typename U>
class store_iterator final
{
public:
	store_iterator (std::nullptr_t)
	{
	}
	store_iterator (std::unique_ptr<vxldollar::store_iterator_impl<T, U>> impl_a) :
		impl (std::move (impl_a))
	{
		impl->fill (current);
	}
	store_iterator (vxldollar::store_iterator<T, U> && other_a) :
		current (std::move (other_a.current)),
		impl (std::move (other_a.impl))
	{
	}
	vxldollar::store_iterator<T, U> & operator++ ()
	{
		++*impl;
		impl->fill (current);
		return *this;
	}
	vxldollar::store_iterator<T, U> & operator-- ()
	{
		--*impl;
		impl->fill (current);
		return *this;
	}
	vxldollar::store_iterator<T, U> & operator= (vxldollar::store_iterator<T, U> && other_a) noexcept
	{
		impl = std::move (other_a.impl);
		current = std::move (other_a.current);
		return *this;
	}
	vxldollar::store_iterator<T, U> & operator= (vxldollar::store_iterator<T, U> const &) = delete;
	std::pair<T, U> * operator-> ()
	{
		return &current;
	}
	bool operator== (vxldollar::store_iterator<T, U> const & other_a) const
	{
		return (impl == nullptr && other_a.impl == nullptr) || (impl != nullptr && *impl == other_a.impl.get ()) || (other_a.impl != nullptr && *other_a.impl == impl.get ());
	}
	bool operator!= (vxldollar::store_iterator<T, U> const & other_a) const
	{
		return !(*this == other_a);
	}

private:
	std::pair<T, U> current;
	std::unique_ptr<vxldollar::store_iterator_impl<T, U>> impl;
};

// Keep this in alphabetical order
enum class tables
{
	accounts,
	blocks,
	confirmation_height,
	default_unused, // RocksDB only
	final_votes,
	frontiers,
	meta,
	online_weight,
	peers,
	pending,
	pruned,
	unchecked,
	vote
};

class transaction_impl
{
public:
	virtual ~transaction_impl () = default;
	virtual void * get_handle () const = 0;
};

class read_transaction_impl : public transaction_impl
{
public:
	virtual void reset () = 0;
	virtual void renew () = 0;
};

class write_transaction_impl : public transaction_impl
{
public:
	virtual void commit () = 0;
	virtual void renew () = 0;
	virtual bool contains (vxldollar::tables table_a) const = 0;
};

class transaction
{
public:
	virtual ~transaction () = default;
	virtual void * get_handle () const = 0;
};

/**
 * RAII wrapper of a read MDB_txn where the constructor starts the transaction
 * and the destructor aborts it.
 */
class read_transaction final : public transaction
{
public:
	explicit read_transaction (std::unique_ptr<vxldollar::read_transaction_impl> read_transaction_impl);
	void * get_handle () const override;
	void reset () const;
	void renew () const;
	void refresh () const;

private:
	std::unique_ptr<vxldollar::read_transaction_impl> impl;
};

/**
 * RAII wrapper of a read-write MDB_txn where the constructor starts the transaction
 * and the destructor commits it.
 */
class write_transaction final : public transaction
{
public:
	explicit write_transaction (std::unique_ptr<vxldollar::write_transaction_impl> write_transaction_impl);
	void * get_handle () const override;
	void commit ();
	void renew ();
	void refresh ();
	bool contains (vxldollar::tables table_a) const;

private:
	std::unique_ptr<vxldollar::write_transaction_impl> impl;
};

class ledger_cache;

/**
 * Manages frontier storage and iteration
 */
class frontier_store
{
public:
	virtual void put (vxldollar::write_transaction const &, vxldollar::block_hash const &, vxldollar::account const &) = 0;
	virtual vxldollar::account get (vxldollar::transaction const &, vxldollar::block_hash const &) const = 0;
	virtual void del (vxldollar::write_transaction const &, vxldollar::block_hash const &) = 0;
	virtual vxldollar::store_iterator<vxldollar::block_hash, vxldollar::account> begin (vxldollar::transaction const &) const = 0;
	virtual vxldollar::store_iterator<vxldollar::block_hash, vxldollar::account> begin (vxldollar::transaction const &, vxldollar::block_hash const &) const = 0;
	virtual vxldollar::store_iterator<vxldollar::block_hash, vxldollar::account> end () const = 0;
	virtual void for_each_par (std::function<void (vxldollar::read_transaction const &, vxldollar::store_iterator<vxldollar::block_hash, vxldollar::account>, vxldollar::store_iterator<vxldollar::block_hash, vxldollar::account>)> const & action_a) const = 0;
};

/**
 * Manages account storage and iteration
 */
class account_store
{
public:
	virtual void put (vxldollar::write_transaction const &, vxldollar::account const &, vxldollar::account_info const &) = 0;
	virtual bool get (vxldollar::transaction const &, vxldollar::account const &, vxldollar::account_info &) = 0;
	virtual void del (vxldollar::write_transaction const &, vxldollar::account const &) = 0;
	virtual bool exists (vxldollar::transaction const &, vxldollar::account const &) = 0;
	virtual size_t count (vxldollar::transaction const &) = 0;
	virtual vxldollar::store_iterator<vxldollar::account, vxldollar::account_info> begin (vxldollar::transaction const &, vxldollar::account const &) const = 0;
	virtual vxldollar::store_iterator<vxldollar::account, vxldollar::account_info> begin (vxldollar::transaction const &) const = 0;
	virtual vxldollar::store_iterator<vxldollar::account, vxldollar::account_info> rbegin (vxldollar::transaction const &) const = 0;
	virtual vxldollar::store_iterator<vxldollar::account, vxldollar::account_info> end () const = 0;
	virtual void for_each_par (std::function<void (vxldollar::read_transaction const &, vxldollar::store_iterator<vxldollar::account, vxldollar::account_info>, vxldollar::store_iterator<vxldollar::account, vxldollar::account_info>)> const &) const = 0;
};

/**
 * Manages pending storage and iteration
 */
class pending_store
{
public:
	virtual void put (vxldollar::write_transaction const &, vxldollar::pending_key const &, vxldollar::pending_info const &) = 0;
	virtual void del (vxldollar::write_transaction const &, vxldollar::pending_key const &) = 0;
	virtual bool get (vxldollar::transaction const &, vxldollar::pending_key const &, vxldollar::pending_info &) = 0;
	virtual bool exists (vxldollar::transaction const &, vxldollar::pending_key const &) = 0;
	virtual bool any (vxldollar::transaction const &, vxldollar::account const &) = 0;
	virtual vxldollar::store_iterator<vxldollar::pending_key, vxldollar::pending_info> begin (vxldollar::transaction const &, vxldollar::pending_key const &) const = 0;
	virtual vxldollar::store_iterator<vxldollar::pending_key, vxldollar::pending_info> begin (vxldollar::transaction const &) const = 0;
	virtual vxldollar::store_iterator<vxldollar::pending_key, vxldollar::pending_info> end () const = 0;
	virtual void for_each_par (std::function<void (vxldollar::read_transaction const &, vxldollar::store_iterator<vxldollar::pending_key, vxldollar::pending_info>, vxldollar::store_iterator<vxldollar::pending_key, vxldollar::pending_info>)> const & action_a) const = 0;
};

/**
 * Manages peer storage and iteration
 */
class peer_store
{
public:
	virtual void put (vxldollar::write_transaction const & transaction_a, vxldollar::endpoint_key const & endpoint_a) = 0;
	virtual void del (vxldollar::write_transaction const & transaction_a, vxldollar::endpoint_key const & endpoint_a) = 0;
	virtual bool exists (vxldollar::transaction const & transaction_a, vxldollar::endpoint_key const & endpoint_a) const = 0;
	virtual size_t count (vxldollar::transaction const & transaction_a) const = 0;
	virtual void clear (vxldollar::write_transaction const & transaction_a) = 0;
	virtual vxldollar::store_iterator<vxldollar::endpoint_key, vxldollar::no_value> begin (vxldollar::transaction const & transaction_a) const = 0;
	virtual vxldollar::store_iterator<vxldollar::endpoint_key, vxldollar::no_value> end () const = 0;
};

/**
 * Manages online weight storage and iteration
 */
class online_weight_store
{
public:
	virtual void put (vxldollar::write_transaction const &, uint64_t, vxldollar::amount const &) = 0;
	virtual void del (vxldollar::write_transaction const &, uint64_t) = 0;
	virtual vxldollar::store_iterator<uint64_t, vxldollar::amount> begin (vxldollar::transaction const &) const = 0;
	virtual vxldollar::store_iterator<uint64_t, vxldollar::amount> rbegin (vxldollar::transaction const &) const = 0;
	virtual vxldollar::store_iterator<uint64_t, vxldollar::amount> end () const = 0;
	virtual size_t count (vxldollar::transaction const &) const = 0;
	virtual void clear (vxldollar::write_transaction const &) = 0;
};

/**
 * Manages pruned storage and iteration
 */
class pruned_store
{
public:
	virtual void put (vxldollar::write_transaction const & transaction_a, vxldollar::block_hash const & hash_a) = 0;
	virtual void del (vxldollar::write_transaction const & transaction_a, vxldollar::block_hash const & hash_a) = 0;
	virtual bool exists (vxldollar::transaction const & transaction_a, vxldollar::block_hash const & hash_a) const = 0;
	virtual vxldollar::block_hash random (vxldollar::transaction const & transaction_a) = 0;
	virtual size_t count (vxldollar::transaction const & transaction_a) const = 0;
	virtual void clear (vxldollar::write_transaction const &) = 0;
	virtual vxldollar::store_iterator<vxldollar::block_hash, std::nullptr_t> begin (vxldollar::transaction const & transaction_a, vxldollar::block_hash const & hash_a) const = 0;
	virtual vxldollar::store_iterator<vxldollar::block_hash, std::nullptr_t> begin (vxldollar::transaction const & transaction_a) const = 0;
	virtual vxldollar::store_iterator<vxldollar::block_hash, std::nullptr_t> end () const = 0;
	virtual void for_each_par (std::function<void (vxldollar::read_transaction const &, vxldollar::store_iterator<vxldollar::block_hash, std::nullptr_t>, vxldollar::store_iterator<vxldollar::block_hash, std::nullptr_t>)> const & action_a) const = 0;
};

/**
 * Manages confirmation height storage and iteration
 */
class confirmation_height_store
{
public:
	virtual void put (vxldollar::write_transaction const & transaction_a, vxldollar::account const & account_a, vxldollar::confirmation_height_info const & confirmation_height_info_a) = 0;

	/** Retrieves confirmation height info relating to an account.
	 *  The parameter confirmation_height_info_a is always written.
	 *  On error, the confirmation height and frontier hash are set to 0.
	 *  Ruturns true on error, false on success.
	 */
	virtual bool get (vxldollar::transaction const & transaction_a, vxldollar::account const & account_a, vxldollar::confirmation_height_info & confirmation_height_info_a) = 0;

	virtual bool exists (vxldollar::transaction const & transaction_a, vxldollar::account const & account_a) const = 0;
	virtual void del (vxldollar::write_transaction const & transaction_a, vxldollar::account const & account_a) = 0;
	virtual uint64_t count (vxldollar::transaction const & transaction_a) = 0;
	virtual void clear (vxldollar::write_transaction const &, vxldollar::account const &) = 0;
	virtual void clear (vxldollar::write_transaction const &) = 0;
	virtual vxldollar::store_iterator<vxldollar::account, vxldollar::confirmation_height_info> begin (vxldollar::transaction const & transaction_a, vxldollar::account const & account_a) const = 0;
	virtual vxldollar::store_iterator<vxldollar::account, vxldollar::confirmation_height_info> begin (vxldollar::transaction const & transaction_a) const = 0;
	virtual vxldollar::store_iterator<vxldollar::account, vxldollar::confirmation_height_info> end () const = 0;
	virtual void for_each_par (std::function<void (vxldollar::read_transaction const &, vxldollar::store_iterator<vxldollar::account, vxldollar::confirmation_height_info>, vxldollar::store_iterator<vxldollar::account, vxldollar::confirmation_height_info>)> const &) const = 0;
};

/**
 * Manages unchecked storage and iteration
 */
class unchecked_store
{
public:
	using iterator = vxldollar::store_iterator<vxldollar::unchecked_key, vxldollar::unchecked_info>;

	virtual void clear (vxldollar::write_transaction const &) = 0;
	virtual void put (vxldollar::write_transaction const &, vxldollar::hash_or_account const & dependency, vxldollar::unchecked_info const &) = 0;
	std::pair<iterator, iterator> equal_range (vxldollar::transaction const & transaction, vxldollar::block_hash const & dependency);
	std::pair<iterator, iterator> full_range (vxldollar::transaction const & transaction);
	std::vector<vxldollar::unchecked_info> get (vxldollar::transaction const &, vxldollar::block_hash const &);
	virtual bool exists (vxldollar::transaction const & transaction_a, vxldollar::unchecked_key const & unchecked_key_a) = 0;
	virtual void del (vxldollar::write_transaction const &, vxldollar::unchecked_key const &) = 0;
	virtual iterator begin (vxldollar::transaction const &) const = 0;
	virtual iterator lower_bound (vxldollar::transaction const &, vxldollar::unchecked_key const &) const = 0;
	virtual iterator end () const = 0;
	virtual size_t count (vxldollar::transaction const &) = 0;
	virtual void for_each_par (std::function<void (vxldollar::read_transaction const &, iterator, iterator)> const & action_a) const = 0;
};

/**
 * Manages final vote storage and iteration
 */
class final_vote_store
{
public:
	virtual bool put (vxldollar::write_transaction const & transaction_a, vxldollar::qualified_root const & root_a, vxldollar::block_hash const & hash_a) = 0;
	virtual std::vector<vxldollar::block_hash> get (vxldollar::transaction const & transaction_a, vxldollar::root const & root_a) = 0;
	virtual void del (vxldollar::write_transaction const & transaction_a, vxldollar::root const & root_a) = 0;
	virtual size_t count (vxldollar::transaction const & transaction_a) const = 0;
	virtual void clear (vxldollar::write_transaction const &, vxldollar::root const &) = 0;
	virtual void clear (vxldollar::write_transaction const &) = 0;
	virtual vxldollar::store_iterator<vxldollar::qualified_root, vxldollar::block_hash> begin (vxldollar::transaction const & transaction_a, vxldollar::qualified_root const & root_a) const = 0;
	virtual vxldollar::store_iterator<vxldollar::qualified_root, vxldollar::block_hash> begin (vxldollar::transaction const & transaction_a) const = 0;
	virtual vxldollar::store_iterator<vxldollar::qualified_root, vxldollar::block_hash> end () const = 0;
	virtual void for_each_par (std::function<void (vxldollar::read_transaction const &, vxldollar::store_iterator<vxldollar::qualified_root, vxldollar::block_hash>, vxldollar::store_iterator<vxldollar::qualified_root, vxldollar::block_hash>)> const & action_a) const = 0;
};

/**
 * Manages version storage
 */
class version_store
{
public:
	virtual void put (vxldollar::write_transaction const &, int) = 0;
	virtual int get (vxldollar::transaction const &) const = 0;
};

/**
 * Manages block storage and iteration
 */
class block_store
{
public:
	virtual void put (vxldollar::write_transaction const &, vxldollar::block_hash const &, vxldollar::block const &) = 0;
	virtual void raw_put (vxldollar::write_transaction const &, std::vector<uint8_t> const &, vxldollar::block_hash const &) = 0;
	virtual vxldollar::block_hash successor (vxldollar::transaction const &, vxldollar::block_hash const &) const = 0;
	virtual void successor_clear (vxldollar::write_transaction const &, vxldollar::block_hash const &) = 0;
	virtual std::shared_ptr<vxldollar::block> get (vxldollar::transaction const &, vxldollar::block_hash const &) const = 0;
	virtual std::shared_ptr<vxldollar::block> get_no_sideband (vxldollar::transaction const &, vxldollar::block_hash const &) const = 0;
	virtual std::shared_ptr<vxldollar::block> random (vxldollar::transaction const &) = 0;
	virtual void del (vxldollar::write_transaction const &, vxldollar::block_hash const &) = 0;
	virtual bool exists (vxldollar::transaction const &, vxldollar::block_hash const &) = 0;
	virtual uint64_t count (vxldollar::transaction const &) = 0;
	virtual vxldollar::account account (vxldollar::transaction const &, vxldollar::block_hash const &) const = 0;
	virtual vxldollar::account account_calculated (vxldollar::block const &) const = 0;
	virtual vxldollar::store_iterator<vxldollar::block_hash, block_w_sideband> begin (vxldollar::transaction const &, vxldollar::block_hash const &) const = 0;
	virtual vxldollar::store_iterator<vxldollar::block_hash, block_w_sideband> begin (vxldollar::transaction const &) const = 0;
	virtual vxldollar::store_iterator<vxldollar::block_hash, block_w_sideband> end () const = 0;
	virtual vxldollar::uint128_t balance (vxldollar::transaction const &, vxldollar::block_hash const &) = 0;
	virtual vxldollar::uint128_t balance_calculated (std::shared_ptr<vxldollar::block> const &) const = 0;
	virtual vxldollar::epoch version (vxldollar::transaction const &, vxldollar::block_hash const &) = 0;
	virtual void for_each_par (std::function<void (vxldollar::read_transaction const &, vxldollar::store_iterator<vxldollar::block_hash, block_w_sideband>, vxldollar::store_iterator<vxldollar::block_hash, block_w_sideband>)> const & action_a) const = 0;
	virtual uint64_t account_height (vxldollar::transaction const & transaction_a, vxldollar::block_hash const & hash_a) const = 0;
};

class unchecked_map;
/**
 * Store manager
 */
class store
{
public:
	// clang-format off
	explicit store (
		vxldollar::block_store &,
		vxldollar::frontier_store &,
		vxldollar::account_store &,
		vxldollar::pending_store &,
		vxldollar::unchecked_store &,
		vxldollar::online_weight_store &,
		vxldollar::pruned_store &,
		vxldollar::peer_store &,
		vxldollar::confirmation_height_store &,
		vxldollar::final_vote_store &,
		vxldollar::version_store &
	);
	// clang-format on
	virtual ~store () = default;
	virtual void initialize (vxldollar::write_transaction const &, vxldollar::ledger_cache &) = 0;
	virtual bool root_exists (vxldollar::transaction const &, vxldollar::root const &) = 0;

	block_store & block;
	frontier_store & frontier;
	account_store & account;
	pending_store & pending;

private:
	unchecked_store & unchecked;

public:
	online_weight_store & online_weight;
	pruned_store & pruned;
	peer_store & peer;
	confirmation_height_store & confirmation_height;
	final_vote_store & final_vote;
	version_store & version;

	virtual unsigned max_block_write_batch_num () const = 0;

	virtual bool copy_db (boost::filesystem::path const & destination) = 0;
	virtual void rebuild_db (vxldollar::write_transaction const & transaction_a) = 0;

	/** Not applicable to all sub-classes */
	virtual void serialize_mdb_tracker (boost::property_tree::ptree &, std::chrono::milliseconds, std::chrono::milliseconds){};
	virtual void serialize_memory_stats (boost::property_tree::ptree &) = 0;

	virtual bool init_error () const = 0;

	/** Start read-write transaction */
	virtual vxldollar::write_transaction tx_begin_write (std::vector<vxldollar::tables> const & tables_to_lock = {}, std::vector<vxldollar::tables> const & tables_no_lock = {}) = 0;

	/** Start read-only transaction */
	virtual vxldollar::read_transaction tx_begin_read () const = 0;

	virtual std::string vendor_get () const = 0;

	friend class unchecked_map;
};

std::unique_ptr<vxldollar::store> make_store (vxldollar::logger_mt & logger, boost::filesystem::path const & path, vxldollar::ledger_constants & constants, bool open_read_only = false, bool add_db_postfix = false, vxldollar::rocksdb_config const & rocksdb_config = vxldollar::rocksdb_config{}, vxldollar::txn_tracking_config const & txn_tracking_config_a = vxldollar::txn_tracking_config{}, std::chrono::milliseconds block_processor_batch_max_time_a = std::chrono::milliseconds (5000), vxldollar::lmdb_config const & lmdb_config_a = vxldollar::lmdb_config{}, bool backup_before_upgrade = false);
}

namespace std
{
template <>
struct hash<::vxldollar::tables>
{
	size_t operator() (::vxldollar::tables const & table_a) const
	{
		return static_cast<size_t> (table_a);
	}
};
}
