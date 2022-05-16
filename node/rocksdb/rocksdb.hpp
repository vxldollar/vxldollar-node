#pragma once

#include <vxldollar/lib/config.hpp>
#include <vxldollar/lib/logger_mt.hpp>
#include <vxldollar/lib/numbers.hpp>
#include <vxldollar/node/rocksdb/rocksdb_iterator.hpp>
#include <vxldollar/secure/common.hpp>
#include <vxldollar/secure/store/account_store_partial.hpp>
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

#include <rocksdb/db.h>
#include <rocksdb/filter_policy.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <rocksdb/table.h>
#include <rocksdb/utilities/optimistic_transaction_db.h>
#include <rocksdb/utilities/transaction.h>

namespace vxldollar
{
class logging_mt;
class rocksdb_config;
class rocksdb_store;

class unchecked_rocksdb_store : public unchecked_store_partial<rocksdb::Slice, vxldollar::rocksdb_store>
{
public:
	explicit unchecked_rocksdb_store (vxldollar::rocksdb_store &);

private:
	vxldollar::rocksdb_store & rocksdb_store;
};

class version_rocksdb_store : public version_store_partial<rocksdb::Slice, vxldollar::rocksdb_store>
{
public:
	explicit version_rocksdb_store (vxldollar::rocksdb_store &);
	void version_put (vxldollar::write_transaction const &, int);

private:
	vxldollar::rocksdb_store & rocksdb_store;
};

/**
 * rocksdb implementation of the block store
 */
class rocksdb_store : public store_partial<rocksdb::Slice, rocksdb_store>
{
private:
	vxldollar::block_store_partial<rocksdb::Slice, rocksdb_store> block_store_partial;
	vxldollar::frontier_store_partial<rocksdb::Slice, rocksdb_store> frontier_store_partial;
	vxldollar::account_store_partial<rocksdb::Slice, rocksdb_store> account_store_partial;
	vxldollar::pending_store_partial<rocksdb::Slice, rocksdb_store> pending_store_partial;
	vxldollar::unchecked_rocksdb_store unchecked_rocksdb_store;
	vxldollar::online_weight_store_partial<rocksdb::Slice, rocksdb_store> online_weight_store_partial;
	vxldollar::pruned_store_partial<rocksdb::Slice, rocksdb_store> pruned_store_partial;
	vxldollar::peer_store_partial<rocksdb::Slice, rocksdb_store> peer_store_partial;
	vxldollar::confirmation_height_store_partial<rocksdb::Slice, rocksdb_store> confirmation_height_store_partial;
	vxldollar::final_vote_store_partial<rocksdb::Slice, rocksdb_store> final_vote_store_partial;
	vxldollar::version_rocksdb_store version_rocksdb_store;

public:
	friend class vxldollar::unchecked_rocksdb_store;
	friend class vxldollar::version_rocksdb_store;

	explicit rocksdb_store (vxldollar::logger_mt &, boost::filesystem::path const &, vxldollar::ledger_constants & constants, vxldollar::rocksdb_config const & = vxldollar::rocksdb_config{}, bool open_read_only = false);

	vxldollar::write_transaction tx_begin_write (std::vector<vxldollar::tables> const & tables_requiring_lock = {}, std::vector<vxldollar::tables> const & tables_no_lock = {}) override;
	vxldollar::read_transaction tx_begin_read () const override;

	std::string vendor_get () const override;

	uint64_t count (vxldollar::transaction const & transaction_a, tables table_a) const override;

	bool exists (vxldollar::transaction const & transaction_a, tables table_a, vxldollar::rocksdb_val const & key_a) const;
	int get (vxldollar::transaction const & transaction_a, tables table_a, vxldollar::rocksdb_val const & key_a, vxldollar::rocksdb_val & value_a) const;
	int put (vxldollar::write_transaction const & transaction_a, tables table_a, vxldollar::rocksdb_val const & key_a, vxldollar::rocksdb_val const & value_a);
	int del (vxldollar::write_transaction const & transaction_a, tables table_a, vxldollar::rocksdb_val const & key_a);

	void serialize_memory_stats (boost::property_tree::ptree &) override;

	bool copy_db (boost::filesystem::path const & destination) override;
	void rebuild_db (vxldollar::write_transaction const & transaction_a) override;

	unsigned max_block_write_batch_num () const override;

	template <typename Key, typename Value>
	vxldollar::store_iterator<Key, Value> make_iterator (vxldollar::transaction const & transaction_a, tables table_a, bool const direction_asc) const
	{
		return vxldollar::store_iterator<Key, Value> (std::make_unique<vxldollar::rocksdb_iterator<Key, Value>> (db.get (), transaction_a, table_to_column_family (table_a), nullptr, direction_asc));
	}

	template <typename Key, typename Value>
	vxldollar::store_iterator<Key, Value> make_iterator (vxldollar::transaction const & transaction_a, tables table_a, vxldollar::rocksdb_val const & key) const
	{
		return vxldollar::store_iterator<Key, Value> (std::make_unique<vxldollar::rocksdb_iterator<Key, Value>> (db.get (), transaction_a, table_to_column_family (table_a), &key, true));
	}

	bool init_error () const override;

	std::string error_string (int status) const override;

private:
	bool error{ false };
	vxldollar::logger_mt & logger;
	vxldollar::ledger_constants & constants;
	// Optimistic transactions are used in write mode
	rocksdb::OptimisticTransactionDB * optimistic_db = nullptr;
	std::unique_ptr<rocksdb::DB> db;
	std::vector<std::unique_ptr<rocksdb::ColumnFamilyHandle>> handles;
	std::shared_ptr<rocksdb::TableFactory> small_table_factory;
	std::unordered_map<vxldollar::tables, vxldollar::mutex> write_lock_mutexes;
	vxldollar::rocksdb_config rocksdb_config;
	unsigned const max_block_write_batch_num_m;

	class tombstone_info
	{
	public:
		tombstone_info (uint64_t, uint64_t const);
		std::atomic<uint64_t> num_since_last_flush;
		uint64_t const max;
	};

	std::unordered_map<vxldollar::tables, tombstone_info> tombstone_map;
	std::unordered_map<char const *, vxldollar::tables> cf_name_table_map;

	rocksdb::Transaction * tx (vxldollar::transaction const & transaction_a) const;
	std::vector<vxldollar::tables> all_tables () const;

	bool not_found (int status) const override;
	bool success (int status) const override;
	int status_code_not_found () const override;
	int drop (vxldollar::write_transaction const &, tables) override;

	rocksdb::ColumnFamilyHandle * table_to_column_family (tables table_a) const;
	int clear (rocksdb::ColumnFamilyHandle * column_family);

	void open (bool & error_a, boost::filesystem::path const & path_a, bool open_read_only_a);

	void construct_column_family_mutexes ();
	rocksdb::Options get_db_options ();
	rocksdb::ColumnFamilyOptions get_common_cf_options (std::shared_ptr<rocksdb::TableFactory> const & table_factory_a, unsigned long long memtable_size_bytes_a) const;
	rocksdb::ColumnFamilyOptions get_active_cf_options (std::shared_ptr<rocksdb::TableFactory> const & table_factory_a, unsigned long long memtable_size_bytes_a) const;
	rocksdb::ColumnFamilyOptions get_small_cf_options (std::shared_ptr<rocksdb::TableFactory> const & table_factory_a) const;
	rocksdb::BlockBasedTableOptions get_active_table_options (std::size_t lru_size) const;
	rocksdb::BlockBasedTableOptions get_small_table_options () const;
	rocksdb::ColumnFamilyOptions get_cf_options (std::string const & cf_name_a) const;

	void on_flush (rocksdb::FlushJobInfo const &);
	void flush_table (vxldollar::tables table_a);
	void flush_tombstones_check (vxldollar::tables table_a);
	void generate_tombstone_map ();
	std::unordered_map<char const *, vxldollar::tables> create_cf_name_table_map () const;

	std::vector<rocksdb::ColumnFamilyDescriptor> create_column_families ();
	unsigned long long base_memtable_size_bytes () const;
	unsigned long long blocks_memtable_size_bytes () const;

	constexpr static int base_memtable_size = 16;
	constexpr static int base_block_cache_size = 8;

	friend class rocksdb_block_store_tombstone_count_Test;
};

extern template class store_partial<rocksdb::Slice, rocksdb_store>;
}
