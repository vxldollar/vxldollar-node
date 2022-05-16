#pragma once

#include <vxldollar/lib/rep_weights.hpp>
#include <vxldollar/lib/timer.hpp>
#include <vxldollar/secure/common.hpp>

#include <map>

namespace vxldollar
{
class store;
class stat;
class write_transaction;

// map of vote weight per block, ordered greater first
using tally_t = std::map<vxldollar::uint128_t, std::shared_ptr<vxldollar::block>, std::greater<vxldollar::uint128_t>>;

class uncemented_info
{
public:
	uncemented_info (vxldollar::block_hash const & cemented_frontier, vxldollar::block_hash const & frontier, vxldollar::account const & account);
	vxldollar::block_hash cemented_frontier;
	vxldollar::block_hash frontier;
	vxldollar::account account;
};

class ledger final
{
public:
	ledger (vxldollar::store &, vxldollar::stat &, vxldollar::ledger_constants & constants, vxldollar::generate_cache const & = vxldollar::generate_cache ());
	vxldollar::account account (vxldollar::transaction const &, vxldollar::block_hash const &) const;
	vxldollar::account account_safe (vxldollar::transaction const &, vxldollar::block_hash const &, bool &) const;
	vxldollar::uint128_t amount (vxldollar::transaction const &, vxldollar::account const &);
	vxldollar::uint128_t amount (vxldollar::transaction const &, vxldollar::block_hash const &);
	/** Safe for previous block, but block hash_a must exist */
	vxldollar::uint128_t amount_safe (vxldollar::transaction const &, vxldollar::block_hash const & hash_a, bool &) const;
	vxldollar::uint128_t balance (vxldollar::transaction const &, vxldollar::block_hash const &) const;
	vxldollar::uint128_t balance_safe (vxldollar::transaction const &, vxldollar::block_hash const &, bool &) const;
	vxldollar::uint128_t account_balance (vxldollar::transaction const &, vxldollar::account const &, bool = false);
	vxldollar::uint128_t account_receivable (vxldollar::transaction const &, vxldollar::account const &, bool = false);
	vxldollar::uint128_t weight (vxldollar::account const &);
	std::shared_ptr<vxldollar::block> successor (vxldollar::transaction const &, vxldollar::qualified_root const &);
	std::shared_ptr<vxldollar::block> forked_block (vxldollar::transaction const &, vxldollar::block const &);
	bool block_confirmed (vxldollar::transaction const &, vxldollar::block_hash const &) const;
	vxldollar::block_hash latest (vxldollar::transaction const &, vxldollar::account const &);
	vxldollar::root latest_root (vxldollar::transaction const &, vxldollar::account const &);
	vxldollar::block_hash representative (vxldollar::transaction const &, vxldollar::block_hash const &);
	vxldollar::block_hash representative_calculated (vxldollar::transaction const &, vxldollar::block_hash const &);
	bool block_or_pruned_exists (vxldollar::block_hash const &) const;
	bool block_or_pruned_exists (vxldollar::transaction const &, vxldollar::block_hash const &) const;
	std::string block_text (char const *);
	std::string block_text (vxldollar::block_hash const &);
	bool is_send (vxldollar::transaction const &, vxldollar::state_block const &) const;
	vxldollar::account const & block_destination (vxldollar::transaction const &, vxldollar::block const &);
	vxldollar::block_hash block_source (vxldollar::transaction const &, vxldollar::block const &);
	std::pair<vxldollar::block_hash, vxldollar::block_hash> hash_root_random (vxldollar::transaction const &) const;
	vxldollar::process_return process (vxldollar::write_transaction const &, vxldollar::block &, vxldollar::signature_verification = vxldollar::signature_verification::unknown);
	bool rollback (vxldollar::write_transaction const &, vxldollar::block_hash const &, std::vector<std::shared_ptr<vxldollar::block>> &);
	bool rollback (vxldollar::write_transaction const &, vxldollar::block_hash const &);
	void update_account (vxldollar::write_transaction const &, vxldollar::account const &, vxldollar::account_info const &, vxldollar::account_info const &);
	uint64_t pruning_action (vxldollar::write_transaction &, vxldollar::block_hash const &, uint64_t const);
	void dump_account_chain (vxldollar::account const &, std::ostream & = std::cout);
	bool could_fit (vxldollar::transaction const &, vxldollar::block const &) const;
	bool dependents_confirmed (vxldollar::transaction const &, vxldollar::block const &) const;
	bool is_epoch_link (vxldollar::link const &) const;
	std::array<vxldollar::block_hash, 2> dependent_blocks (vxldollar::transaction const &, vxldollar::block const &) const;
	std::shared_ptr<vxldollar::block> find_receive_block_by_send_hash (vxldollar::transaction const & transaction, vxldollar::account const & destination, vxldollar::block_hash const & send_block_hash);
	vxldollar::account const & epoch_signer (vxldollar::link const &) const;
	vxldollar::link const & epoch_link (vxldollar::epoch) const;
	std::multimap<uint64_t, uncemented_info, std::greater<>> unconfirmed_frontiers () const;
	bool migrate_lmdb_to_rocksdb (boost::filesystem::path const &) const;
	static vxldollar::uint128_t const unit;
	vxldollar::ledger_constants & constants;
	vxldollar::store & store;
	vxldollar::ledger_cache cache;
	vxldollar::stat & stats;
	std::unordered_map<vxldollar::account, vxldollar::uint128_t> bootstrap_weights;
	std::atomic<size_t> bootstrap_weights_size{ 0 };
	uint64_t bootstrap_weight_max_blocks{ 1 };
	std::atomic<bool> check_bootstrap_weights;
	bool pruning{ false };

private:
	void initialize (vxldollar::generate_cache const &);
};

std::unique_ptr<container_info_component> collect_container_info (ledger & ledger, std::string const & name);
}
