#pragma once

#include <vxldollar/lib/diagnosticsconfig.hpp>
#include <vxldollar/lib/lmdbconfig.hpp>
#include <vxldollar/lib/logger_mt.hpp>
#include <vxldollar/lib/numbers.hpp>
#include <vxldollar/node/lmdb/lmdb_env.hpp>
#include <vxldollar/node/lmdb/lmdb_iterator.hpp>
#include <vxldollar/node/lmdb/lmdb_txn.hpp>
#include <vxldollar/secure/common.hpp>
#include <vxldollar/secure/store/account_store_partial.hpp>
#include <vxldollar/secure/store/block_store_partial.hpp>
#include <vxldollar/secure/store/confirmation_height_store_partial.hpp>
#include <vxldollar/secure/store/final_vote_store_partial.hpp>
#include <vxldollar/secure/store/frontier_store_partial.hpp>
#include <vxldollar/secure/store/online_weight_partial.hpp>
#include <vxldollar/secure/store/peer_store_partial.hpp>
#include <vxldollar/secure/store/pending_store_partial.hpp>
#include <vxldollar/secure/store/pruned_store_partial.hpp>
#include <vxldollar/secure/store/unchecked_store_partial.hpp>
#include <vxldollar/secure/store/version_store_partial.hpp>
#include <vxldollar/secure/store_partial.hpp>
#include <vxldollar/secure/versioning.hpp>

#include <boost/optional.hpp>

#include <lmdb/libraries/liblmdb/lmdb.h>

namespace boost
{
namespace filesystem
{
	class path;
}
}

namespace vxldollar
{
using mdb_val = db_val<MDB_val>;

class logging_mt;
class mdb_store;

class unchecked_mdb_store : public unchecked_store_partial<MDB_val, mdb_store>
{
public:
	explicit unchecked_mdb_store (vxldollar::mdb_store &);
};

/**
 * mdb implementation of the block store
 */
class mdb_store : public store_partial<MDB_val, mdb_store>
{
private:
	vxldollar::block_store_partial<MDB_val, mdb_store> block_store_partial;
	vxldollar::frontier_store_partial<MDB_val, mdb_store> frontier_store_partial;
	vxldollar::account_store_partial<MDB_val, mdb_store> account_store_partial;
	vxldollar::pending_store_partial<MDB_val, mdb_store> pending_store_partial;
	vxldollar::unchecked_mdb_store unchecked_mdb_store;
	vxldollar::online_weight_store_partial<MDB_val, mdb_store> online_weight_store_partial;
	vxldollar::pruned_store_partial<MDB_val, mdb_store> pruned_store_partial;
	vxldollar::peer_store_partial<MDB_val, mdb_store> peer_store_partial;
	vxldollar::confirmation_height_store_partial<MDB_val, mdb_store> confirmation_height_store_partial;
	vxldollar::final_vote_store_partial<MDB_val, mdb_store> final_vote_store_partial;
	vxldollar::version_store_partial<MDB_val, mdb_store> version_store_partial;

	friend class vxldollar::unchecked_mdb_store;

public:
	mdb_store (vxldollar::logger_mt &, boost::filesystem::path const &, vxldollar::ledger_constants & constants, vxldollar::txn_tracking_config const & txn_tracking_config_a = vxldollar::txn_tracking_config{}, std::chrono::milliseconds block_processor_batch_max_time_a = std::chrono::milliseconds (5000), vxldollar::lmdb_config const & lmdb_config_a = vxldollar::lmdb_config{}, bool backup_before_upgrade = false);
	vxldollar::write_transaction tx_begin_write (std::vector<vxldollar::tables> const & tables_requiring_lock = {}, std::vector<vxldollar::tables> const & tables_no_lock = {}) override;
	vxldollar::read_transaction tx_begin_read () const override;

	std::string vendor_get () const override;

	void serialize_mdb_tracker (boost::property_tree::ptree &, std::chrono::milliseconds, std::chrono::milliseconds) override;

	static void create_backup_file (vxldollar::mdb_env &, boost::filesystem::path const &, vxldollar::logger_mt &);

	void serialize_memory_stats (boost::property_tree::ptree &) override;

	unsigned max_block_write_batch_num () const override;

private:
	vxldollar::logger_mt & logger;
	bool error{ false };

public:
	vxldollar::mdb_env env;

	/**
	 * Maps head block to owning account
	 * vxldollar::block_hash -> vxldollar::account
	 */
	MDB_dbi frontiers_handle{ 0 };

	/**
	 * Maps account v1 to account information, head, rep, open, balance, timestamp and block count. (Removed)
	 * vxldollar::account -> vxldollar::block_hash, vxldollar::block_hash, vxldollar::block_hash, vxldollar::amount, uint64_t, uint64_t
	 */
	MDB_dbi accounts_v0_handle{ 0 };

	/**
	 * Maps account v0 to account information, head, rep, open, balance, timestamp and block count. (Removed)
	 * vxldollar::account -> vxldollar::block_hash, vxldollar::block_hash, vxldollar::block_hash, vxldollar::amount, uint64_t, uint64_t
	 */
	MDB_dbi accounts_v1_handle{ 0 };

	/**
	 * Maps account v0 to account information, head, rep, open, balance, timestamp, block count and epoch
	 * vxldollar::account -> vxldollar::block_hash, vxldollar::block_hash, vxldollar::block_hash, vxldollar::amount, uint64_t, uint64_t, vxldollar::epoch
	 */
	MDB_dbi accounts_handle{ 0 };

	/**
	 * Maps block hash to send block. (Removed)
	 * vxldollar::block_hash -> vxldollar::send_block
	 */
	MDB_dbi send_blocks_handle{ 0 };

	/**
	 * Maps block hash to receive block. (Removed)
	 * vxldollar::block_hash -> vxldollar::receive_block
	 */
	MDB_dbi receive_blocks_handle{ 0 };

	/**
	 * Maps block hash to open block. (Removed)
	 * vxldollar::block_hash -> vxldollar::open_block
	 */
	MDB_dbi open_blocks_handle{ 0 };

	/**
	 * Maps block hash to change block. (Removed)
	 * vxldollar::block_hash -> vxldollar::change_block
	 */
	MDB_dbi change_blocks_handle{ 0 };

	/**
	 * Maps block hash to v0 state block. (Removed)
	 * vxldollar::block_hash -> vxldollar::state_block
	 */
	MDB_dbi state_blocks_v0_handle{ 0 };

	/**
	 * Maps block hash to v1 state block. (Removed)
	 * vxldollar::block_hash -> vxldollar::state_block
	 */
	MDB_dbi state_blocks_v1_handle{ 0 };

	/**
	 * Maps block hash to state block. (Removed)
	 * vxldollar::block_hash -> vxldollar::state_block
	 */
	MDB_dbi state_blocks_handle{ 0 };

	/**
	 * Maps min_version 0 (destination account, pending block) to (source account, amount). (Removed)
	 * vxldollar::account, vxldollar::block_hash -> vxldollar::account, vxldollar::amount
	 */
	MDB_dbi pending_v0_handle{ 0 };

	/**
	 * Maps min_version 1 (destination account, pending block) to (source account, amount). (Removed)
	 * vxldollar::account, vxldollar::block_hash -> vxldollar::account, vxldollar::amount
	 */
	MDB_dbi pending_v1_handle{ 0 };

	/**
	 * Maps (destination account, pending block) to (source account, amount, version). (Removed)
	 * vxldollar::account, vxldollar::block_hash -> vxldollar::account, vxldollar::amount, vxldollar::epoch
	 */
	MDB_dbi pending_handle{ 0 };

	/**
	 * Representative weights. (Removed)
	 * vxldollar::account -> vxldollar::uint128_t
	 */
	MDB_dbi representation_handle{ 0 };

	/**
	 * Unchecked bootstrap blocks info.
	 * vxldollar::block_hash -> vxldollar::unchecked_info
	 */
	MDB_dbi unchecked_handle{ 0 };

	/**
	 * Samples of online vote weight
	 * uint64_t -> vxldollar::amount
	 */
	MDB_dbi online_weight_handle{ 0 };

	/**
	 * Meta information about block store, such as versions.
	 * vxldollar::uint256_union (arbitrary key) -> blob
	 */
	MDB_dbi meta_handle{ 0 };

	/**
	 * Pruned blocks hashes
	 * vxldollar::block_hash -> none
	 */
	MDB_dbi pruned_handle{ 0 };

	/*
	 * Endpoints for peers
	 * vxldollar::endpoint_key -> no_value
	*/
	MDB_dbi peers_handle{ 0 };

	/*
	 * Confirmation height of an account, and the hash for the block at that height
	 * vxldollar::account -> uint64_t, vxldollar::block_hash
	 */
	MDB_dbi confirmation_height_handle{ 0 };

	/*
	 * Contains block_sideband and block for all block types (legacy send/change/open/receive & state blocks)
	 * vxldollar::block_hash -> vxldollar::block_sideband, vxldollar::block
	 */
	MDB_dbi blocks_handle{ 0 };

	/**
	 * Maps root to block hash for generated final votes.
	 * vxldollar::qualified_root -> vxldollar::block_hash
	 */
	MDB_dbi final_votes_handle{ 0 };

	bool exists (vxldollar::transaction const & transaction_a, tables table_a, vxldollar::mdb_val const & key_a) const;

	int get (vxldollar::transaction const & transaction_a, tables table_a, vxldollar::mdb_val const & key_a, vxldollar::mdb_val & value_a) const;
	int put (vxldollar::write_transaction const & transaction_a, tables table_a, vxldollar::mdb_val const & key_a, vxldollar::mdb_val const & value_a) const;
	int del (vxldollar::write_transaction const & transaction_a, tables table_a, vxldollar::mdb_val const & key_a) const;

	bool copy_db (boost::filesystem::path const & destination_file) override;
	void rebuild_db (vxldollar::write_transaction const & transaction_a) override;

	template <typename Key, typename Value>
	vxldollar::store_iterator<Key, Value> make_iterator (vxldollar::transaction const & transaction_a, tables table_a, bool const direction_asc) const
	{
		return vxldollar::store_iterator<Key, Value> (std::make_unique<vxldollar::mdb_iterator<Key, Value>> (transaction_a, table_to_dbi (table_a), vxldollar::mdb_val{}, direction_asc));
	}

	template <typename Key, typename Value>
	vxldollar::store_iterator<Key, Value> make_iterator (vxldollar::transaction const & transaction_a, tables table_a, vxldollar::mdb_val const & key) const
	{
		return vxldollar::store_iterator<Key, Value> (std::make_unique<vxldollar::mdb_iterator<Key, Value>> (transaction_a, table_to_dbi (table_a), key));
	}

	bool init_error () const override;

	uint64_t count (vxldollar::transaction const &, MDB_dbi) const;
	std::string error_string (int status) const override;

	// These are only use in the upgrade process.
	std::shared_ptr<vxldollar::block> block_get_v14 (vxldollar::transaction const & transaction_a, vxldollar::block_hash const & hash_a, vxldollar::block_sideband_v14 * sideband_a = nullptr, bool * is_state_v1 = nullptr) const;
	std::size_t block_successor_offset_v14 (vxldollar::transaction const & transaction_a, std::size_t entry_size_a, vxldollar::block_type type_a) const;
	vxldollar::block_hash block_successor_v14 (vxldollar::transaction const & transaction_a, vxldollar::block_hash const & hash_a) const;
	vxldollar::mdb_val block_raw_get_v14 (vxldollar::transaction const & transaction_a, vxldollar::block_hash const & hash_a, vxldollar::block_type & type_a, bool * is_state_v1 = nullptr) const;
	boost::optional<vxldollar::mdb_val> block_raw_get_by_type_v14 (vxldollar::transaction const & transaction_a, vxldollar::block_hash const & hash_a, vxldollar::block_type & type_a, bool * is_state_v1) const;

private:
	bool do_upgrades (vxldollar::write_transaction &, bool &);
	void upgrade_v14_to_v15 (vxldollar::write_transaction &);
	void upgrade_v15_to_v16 (vxldollar::write_transaction const &);
	void upgrade_v16_to_v17 (vxldollar::write_transaction const &);
	void upgrade_v17_to_v18 (vxldollar::write_transaction const &);
	void upgrade_v18_to_v19 (vxldollar::write_transaction const &);
	void upgrade_v19_to_v20 (vxldollar::write_transaction const &);
	void upgrade_v20_to_v21 (vxldollar::write_transaction const &);

	std::shared_ptr<vxldollar::block> block_get_v18 (vxldollar::transaction const & transaction_a, vxldollar::block_hash const & hash_a) const;
	vxldollar::mdb_val block_raw_get_v18 (vxldollar::transaction const & transaction_a, vxldollar::block_hash const & hash_a, vxldollar::block_type & type_a) const;
	boost::optional<vxldollar::mdb_val> block_raw_get_by_type_v18 (vxldollar::transaction const & transaction_a, vxldollar::block_hash const & hash_a, vxldollar::block_type & type_a) const;
	vxldollar::uint128_t block_balance_v18 (vxldollar::transaction const & transaction_a, vxldollar::block_hash const & hash_a) const;

	void open_databases (bool &, vxldollar::transaction const &, unsigned);

	int drop (vxldollar::write_transaction const & transaction_a, tables table_a) override;
	int clear (vxldollar::write_transaction const & transaction_a, MDB_dbi handle_a);

	bool not_found (int status) const override;
	bool success (int status) const override;
	int status_code_not_found () const override;

	MDB_dbi table_to_dbi (tables table_a) const;

	mutable vxldollar::mdb_txn_tracker mdb_txn_tracker;
	vxldollar::mdb_txn_callbacks create_txn_callbacks () const;
	bool txn_tracking_enabled;

	uint64_t count (vxldollar::transaction const & transaction_a, tables table_a) const override;

	bool vacuum_after_upgrade (boost::filesystem::path const & path_a, vxldollar::lmdb_config const & lmdb_config_a);

	class upgrade_counters
	{
	public:
		upgrade_counters (uint64_t count_before_v0, uint64_t count_before_v1);
		bool are_equal () const;

		uint64_t before_v0;
		uint64_t before_v1;
		uint64_t after_v0{ 0 };
		uint64_t after_v1{ 0 };
	};
};

template <>
void * mdb_val::data () const;
template <>
std::size_t mdb_val::size () const;
template <>
mdb_val::db_val (std::size_t size_a, void * data_a);
template <>
void mdb_val::convert_buffer_to_value ();

extern template class store_partial<MDB_val, mdb_store>;
}
