#pragma once

#include <vxldollar/node/bootstrap/bootstrap_attempt.hpp>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>

#include <atomic>
#include <queue>
#include <unordered_set>

namespace mi = boost::multi_index;

namespace vxldollar
{
class node;
class lazy_state_backlog_item final
{
public:
	vxldollar::link link{ 0 };
	vxldollar::uint128_t balance{ 0 };
	unsigned retry_limit{ 0 };
};

/**
 * Lazy bootstrap session. Started with a block hash, this will "trace down" the blocks obtained to find a connection to the ledger.
 * This attempts to quickly bootstrap a section of the ledger given a hash that's known to be confirmed.
 */
class bootstrap_attempt_lazy final : public bootstrap_attempt
{
public:
	explicit bootstrap_attempt_lazy (std::shared_ptr<vxldollar::node> const & node_a, uint64_t incremental_id_a, std::string const & id_a = "");
	~bootstrap_attempt_lazy ();
	bool process_block (std::shared_ptr<vxldollar::block> const &, vxldollar::account const &, uint64_t, vxldollar::bulk_pull::count_t, bool, unsigned) override;
	void run () override;
	bool lazy_start (vxldollar::hash_or_account const &, bool confirmed = true) override;
	void lazy_add (vxldollar::hash_or_account const &, unsigned);
	void lazy_add (vxldollar::pull_info const &) override;
	void lazy_requeue (vxldollar::block_hash const &, vxldollar::block_hash const &) override;
	bool lazy_finished ();
	bool lazy_has_expired () const override;
	uint32_t lazy_batch_size () override;
	void lazy_pull_flush (vxldollar::unique_lock<vxldollar::mutex> & lock_a);
	bool process_block_lazy (std::shared_ptr<vxldollar::block> const &, vxldollar::account const &, uint64_t, vxldollar::bulk_pull::count_t, unsigned);
	void lazy_block_state (std::shared_ptr<vxldollar::block> const &, unsigned);
	void lazy_block_state_backlog_check (std::shared_ptr<vxldollar::block> const &, vxldollar::block_hash const &);
	void lazy_backlog_cleanup ();
	void lazy_blocks_insert (vxldollar::block_hash const &);
	void lazy_blocks_erase (vxldollar::block_hash const &);
	bool lazy_blocks_processed (vxldollar::block_hash const &);
	bool lazy_processed_or_exists (vxldollar::block_hash const &) override;
	unsigned lazy_retry_limit_confirmed ();
	void get_information (boost::property_tree::ptree &) override;
	std::unordered_set<std::size_t> lazy_blocks;
	std::unordered_map<vxldollar::block_hash, vxldollar::lazy_state_backlog_item> lazy_state_backlog;
	std::unordered_set<vxldollar::block_hash> lazy_undefined_links;
	std::unordered_map<vxldollar::block_hash, vxldollar::uint128_t> lazy_balances;
	std::unordered_set<vxldollar::block_hash> lazy_keys;
	std::deque<std::pair<vxldollar::hash_or_account, unsigned>> lazy_pulls;
	std::chrono::steady_clock::time_point lazy_start_time;
	std::atomic<std::size_t> lazy_blocks_count{ 0 };
	std::size_t peer_count{ 0 };
	/** The maximum number of records to be read in while iterating over long lazy containers */
	static uint64_t constexpr batch_read_size = 256;
};

/**
 * Wallet bootstrap session. This session will trace down accounts within local wallets to try and bootstrap those blocks first.
 */
class bootstrap_attempt_wallet final : public bootstrap_attempt
{
public:
	explicit bootstrap_attempt_wallet (std::shared_ptr<vxldollar::node> const & node_a, uint64_t incremental_id_a, std::string id_a = "");
	~bootstrap_attempt_wallet ();
	void request_pending (vxldollar::unique_lock<vxldollar::mutex> &);
	void requeue_pending (vxldollar::account const &) override;
	void run () override;
	void wallet_start (std::deque<vxldollar::account> &) override;
	bool wallet_finished ();
	std::size_t wallet_size () override;
	void get_information (boost::property_tree::ptree &) override;
	std::deque<vxldollar::account> wallet_accounts;
};
}
