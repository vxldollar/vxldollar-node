#pragma once

#include <vxldollar/lib/lmdbconfig.hpp>
#include <vxldollar/lib/locks.hpp>
#include <vxldollar/lib/work.hpp>
#include <vxldollar/node/lmdb/lmdb.hpp>
#include <vxldollar/node/lmdb/wallet_value.hpp>
#include <vxldollar/node/openclwork.hpp>
#include <vxldollar/secure/common.hpp>
#include <vxldollar/secure/store.hpp>

#include <atomic>
#include <mutex>
#include <thread>
#include <unordered_set>
namespace vxldollar
{
class node;
class node_config;
class wallets;
// The fan spreads a key out over the heap to decrease the likelihood of it being recovered by memory inspection
class fan final
{
public:
	fan (vxldollar::raw_key const &, std::size_t);
	void value (vxldollar::raw_key &);
	void value_set (vxldollar::raw_key const &);
	std::vector<std::unique_ptr<vxldollar::raw_key>> values;

private:
	vxldollar::mutex mutex;
	void value_get (vxldollar::raw_key &);
};
class kdf final
{
public:
	kdf (unsigned & kdf_work) :
		kdf_work{ kdf_work }
	{
	}
	void phs (vxldollar::raw_key &, std::string const &, vxldollar::uint256_union const &);
	vxldollar::mutex mutex;
	unsigned & kdf_work;
};
enum class key_type
{
	not_a_type,
	unknown,
	adhoc,
	deterministic
};
class wallet_store final
{
public:
	wallet_store (bool &, vxldollar::kdf &, vxldollar::transaction &, vxldollar::account, unsigned, std::string const &);
	wallet_store (bool &, vxldollar::kdf &, vxldollar::transaction &, vxldollar::account, unsigned, std::string const &, std::string const &);
	std::vector<vxldollar::account> accounts (vxldollar::transaction const &);
	void initialize (vxldollar::transaction const &, bool &, std::string const &);
	vxldollar::uint256_union check (vxldollar::transaction const &);
	bool rekey (vxldollar::transaction const &, std::string const &);
	bool valid_password (vxldollar::transaction const &);
	bool valid_public_key (vxldollar::public_key const &);
	bool attempt_password (vxldollar::transaction const &, std::string const &);
	void wallet_key (vxldollar::raw_key &, vxldollar::transaction const &);
	void seed (vxldollar::raw_key &, vxldollar::transaction const &);
	void seed_set (vxldollar::transaction const &, vxldollar::raw_key const &);
	vxldollar::key_type key_type (vxldollar::wallet_value const &);
	vxldollar::public_key deterministic_insert (vxldollar::transaction const &);
	vxldollar::public_key deterministic_insert (vxldollar::transaction const &, uint32_t const);
	vxldollar::raw_key deterministic_key (vxldollar::transaction const &, uint32_t);
	uint32_t deterministic_index_get (vxldollar::transaction const &);
	void deterministic_index_set (vxldollar::transaction const &, uint32_t);
	void deterministic_clear (vxldollar::transaction const &);
	vxldollar::uint256_union salt (vxldollar::transaction const &);
	bool is_representative (vxldollar::transaction const &);
	vxldollar::account representative (vxldollar::transaction const &);
	void representative_set (vxldollar::transaction const &, vxldollar::account const &);
	vxldollar::public_key insert_adhoc (vxldollar::transaction const &, vxldollar::raw_key const &);
	bool insert_watch (vxldollar::transaction const &, vxldollar::account const &);
	void erase (vxldollar::transaction const &, vxldollar::account const &);
	vxldollar::wallet_value entry_get_raw (vxldollar::transaction const &, vxldollar::account const &);
	void entry_put_raw (vxldollar::transaction const &, vxldollar::account const &, vxldollar::wallet_value const &);
	bool fetch (vxldollar::transaction const &, vxldollar::account const &, vxldollar::raw_key &);
	bool exists (vxldollar::transaction const &, vxldollar::account const &);
	void destroy (vxldollar::transaction const &);
	vxldollar::store_iterator<vxldollar::account, vxldollar::wallet_value> find (vxldollar::transaction const &, vxldollar::account const &);
	vxldollar::store_iterator<vxldollar::account, vxldollar::wallet_value> begin (vxldollar::transaction const &, vxldollar::account const &);
	vxldollar::store_iterator<vxldollar::account, vxldollar::wallet_value> begin (vxldollar::transaction const &);
	vxldollar::store_iterator<vxldollar::account, vxldollar::wallet_value> end ();
	void derive_key (vxldollar::raw_key &, vxldollar::transaction const &, std::string const &);
	void serialize_json (vxldollar::transaction const &, std::string &);
	void write_backup (vxldollar::transaction const &, boost::filesystem::path const &);
	bool move (vxldollar::transaction const &, vxldollar::wallet_store &, std::vector<vxldollar::public_key> const &);
	bool import (vxldollar::transaction const &, vxldollar::wallet_store &);
	bool work_get (vxldollar::transaction const &, vxldollar::public_key const &, uint64_t &);
	void work_put (vxldollar::transaction const &, vxldollar::public_key const &, uint64_t);
	unsigned version (vxldollar::transaction const &);
	void version_put (vxldollar::transaction const &, unsigned);
	vxldollar::fan password;
	vxldollar::fan wallet_key_mem;
	static unsigned const version_1 = 1;
	static unsigned const version_2 = 2;
	static unsigned const version_3 = 3;
	static unsigned const version_4 = 4;
	static unsigned constexpr version_current = version_4;
	static vxldollar::account const version_special;
	static vxldollar::account const wallet_key_special;
	static vxldollar::account const salt_special;
	static vxldollar::account const check_special;
	static vxldollar::account const representative_special;
	static vxldollar::account const seed_special;
	static vxldollar::account const deterministic_index_special;
	static std::size_t const check_iv_index;
	static std::size_t const seed_iv_index;
	static int const special_count;
	vxldollar::kdf & kdf;
	std::atomic<MDB_dbi> handle{ 0 };
	std::recursive_mutex mutex;

private:
	MDB_txn * tx (vxldollar::transaction const &) const;
};
// A wallet is a set of account keys encrypted by a common encryption key
class wallet final : public std::enable_shared_from_this<vxldollar::wallet>
{
public:
	std::shared_ptr<vxldollar::block> change_action (vxldollar::account const &, vxldollar::account const &, uint64_t = 0, bool = true);
	std::shared_ptr<vxldollar::block> receive_action (vxldollar::block_hash const &, vxldollar::account const &, vxldollar::uint128_union const &, vxldollar::account const &, uint64_t = 0, bool = true);
	std::shared_ptr<vxldollar::block> send_action (vxldollar::account const &, vxldollar::account const &, vxldollar::uint128_t const &, uint64_t = 0, bool = true, boost::optional<std::string> = {});
	bool action_complete (std::shared_ptr<vxldollar::block> const &, vxldollar::account const &, bool const, vxldollar::block_details const &);
	wallet (bool &, vxldollar::transaction &, vxldollar::wallets &, std::string const &);
	wallet (bool &, vxldollar::transaction &, vxldollar::wallets &, std::string const &, std::string const &);
	void enter_initial_password ();
	bool enter_password (vxldollar::transaction const &, std::string const &);
	vxldollar::public_key insert_adhoc (vxldollar::raw_key const &, bool = true);
	bool insert_watch (vxldollar::transaction const &, vxldollar::public_key const &);
	vxldollar::public_key deterministic_insert (vxldollar::transaction const &, bool = true);
	vxldollar::public_key deterministic_insert (uint32_t, bool = true);
	vxldollar::public_key deterministic_insert (bool = true);
	bool exists (vxldollar::public_key const &);
	bool import (std::string const &, std::string const &);
	void serialize (std::string &);
	bool change_sync (vxldollar::account const &, vxldollar::account const &);
	void change_async (vxldollar::account const &, vxldollar::account const &, std::function<void (std::shared_ptr<vxldollar::block> const &)> const &, uint64_t = 0, bool = true);
	bool receive_sync (std::shared_ptr<vxldollar::block> const &, vxldollar::account const &, vxldollar::uint128_t const &);
	void receive_async (vxldollar::block_hash const &, vxldollar::account const &, vxldollar::uint128_t const &, vxldollar::account const &, std::function<void (std::shared_ptr<vxldollar::block> const &)> const &, uint64_t = 0, bool = true);
	vxldollar::block_hash send_sync (vxldollar::account const &, vxldollar::account const &, vxldollar::uint128_t const &);
	void send_async (vxldollar::account const &, vxldollar::account const &, vxldollar::uint128_t const &, std::function<void (std::shared_ptr<vxldollar::block> const &)> const &, uint64_t = 0, bool = true, boost::optional<std::string> = {});
	void work_cache_blocking (vxldollar::account const &, vxldollar::root const &);
	void work_update (vxldollar::transaction const &, vxldollar::account const &, vxldollar::root const &, uint64_t);
	// Schedule work generation after a few seconds
	void work_ensure (vxldollar::account const &, vxldollar::root const &);
	bool search_receivable (vxldollar::transaction const &);
	void init_free_accounts (vxldollar::transaction const &);
	uint32_t deterministic_check (vxldollar::transaction const & transaction_a, uint32_t index);
	/** Changes the wallet seed and returns the first account */
	vxldollar::public_key change_seed (vxldollar::transaction const & transaction_a, vxldollar::raw_key const & prv_a, uint32_t count = 0);
	void deterministic_restore (vxldollar::transaction const & transaction_a);
	bool live ();
	std::unordered_set<vxldollar::account> free_accounts;
	std::function<void (bool, bool)> lock_observer;
	vxldollar::wallet_store store;
	vxldollar::wallets & wallets;
	vxldollar::mutex representatives_mutex;
	std::unordered_set<vxldollar::account> representatives;
};

class wallet_representatives
{
public:
	uint64_t voting{ 0 }; // Number of representatives with at least the configured minimum voting weight
	bool half_principal{ false }; // has representatives with at least 50% of principal representative requirements
	std::unordered_set<vxldollar::account> accounts; // Representatives with at least the configured minimum voting weight
	bool have_half_rep () const
	{
		return half_principal;
	}
	bool exists (vxldollar::account const & rep_a) const
	{
		return accounts.count (rep_a) > 0;
	}
	void clear ()
	{
		voting = 0;
		half_principal = false;
		accounts.clear ();
	}
};

/**
 * The wallets set is all the wallets a node controls.
 * A node may contain multiple wallets independently encrypted and operated.
 */
class wallets final
{
public:
	wallets (bool, vxldollar::node &);
	~wallets ();
	std::shared_ptr<vxldollar::wallet> open (vxldollar::wallet_id const &);
	std::shared_ptr<vxldollar::wallet> create (vxldollar::wallet_id const &);
	bool search_receivable (vxldollar::wallet_id const &);
	void search_receivable_all ();
	void destroy (vxldollar::wallet_id const &);
	void reload ();
	void do_wallet_actions ();
	void queue_wallet_action (vxldollar::uint128_t const &, std::shared_ptr<vxldollar::wallet> const &, std::function<void (vxldollar::wallet &)>);
	void foreach_representative (std::function<void (vxldollar::public_key const &, vxldollar::raw_key const &)> const &);
	bool exists (vxldollar::transaction const &, vxldollar::account const &);
	void start ();
	void stop ();
	void clear_send_ids (vxldollar::transaction const &);
	vxldollar::wallet_representatives reps () const;
	bool check_rep (vxldollar::account const &, vxldollar::uint128_t const &, bool const = true);
	void compute_reps ();
	void ongoing_compute_reps ();
	void split_if_needed (vxldollar::transaction &, vxldollar::store &);
	void move_table (std::string const &, MDB_txn *, MDB_txn *);
	std::unordered_map<vxldollar::wallet_id, std::shared_ptr<vxldollar::wallet>> get_wallets ();
	vxldollar::network_params & network_params;
	std::function<void (bool)> observer;
	std::unordered_map<vxldollar::wallet_id, std::shared_ptr<vxldollar::wallet>> items;
	std::multimap<vxldollar::uint128_t, std::pair<std::shared_ptr<vxldollar::wallet>, std::function<void (vxldollar::wallet &)>>, std::greater<vxldollar::uint128_t>> actions;
	vxldollar::locked<std::unordered_map<vxldollar::account, vxldollar::root>> delayed_work;
	vxldollar::mutex mutex;
	vxldollar::mutex action_mutex;
	vxldollar::condition_variable condition;
	vxldollar::kdf kdf;
	MDB_dbi handle;
	MDB_dbi send_action_ids;
	vxldollar::node & node;
	vxldollar::mdb_env & env;
	std::atomic<bool> stopped;
	std::thread thread;
	static vxldollar::uint128_t const generate_priority;
	static vxldollar::uint128_t const high_priority;
	/** Start read-write transaction */
	vxldollar::write_transaction tx_begin_write ();

	/** Start read-only transaction */
	vxldollar::read_transaction tx_begin_read ();

private:
	mutable vxldollar::mutex reps_cache_mutex;
	vxldollar::wallet_representatives representatives;
};

std::unique_ptr<container_info_component> collect_container_info (wallets & wallets, std::string const & name);

class wallets_store
{
public:
	virtual ~wallets_store () = default;
	virtual bool init_error () const = 0;
};
class mdb_wallets_store final : public wallets_store
{
public:
	mdb_wallets_store (boost::filesystem::path const &, vxldollar::lmdb_config const & lmdb_config_a = vxldollar::lmdb_config{});
	vxldollar::mdb_env environment;
	bool init_error () const override;
	bool error{ false };
};
}
