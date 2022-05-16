#pragma once

#include <vxldollar/crypto/blake2/blake2.h>
#include <vxldollar/lib/blockbuilders.hpp>
#include <vxldollar/lib/blocks.hpp>
#include <vxldollar/lib/config.hpp>
#include <vxldollar/lib/epoch.hpp>
#include <vxldollar/lib/numbers.hpp>
#include <vxldollar/lib/rep_weights.hpp>
#include <vxldollar/lib/utility.hpp>

#include <boost/iterator/transform_iterator.hpp>
#include <boost/optional/optional.hpp>
#include <boost/property_tree/ptree_fwd.hpp>
#include <boost/variant/variant.hpp>

#include <unordered_map>

namespace boost
{
template <>
struct hash<::vxldollar::uint256_union>
{
	size_t operator() (::vxldollar::uint256_union const & value_a) const
	{
		return std::hash<::vxldollar::uint256_union> () (value_a);
	}
};

template <>
struct hash<::vxldollar::block_hash>
{
	size_t operator() (::vxldollar::block_hash const & value_a) const
	{
		return std::hash<::vxldollar::block_hash> () (value_a);
	}
};

template <>
struct hash<::vxldollar::public_key>
{
	size_t operator() (::vxldollar::public_key const & value_a) const
	{
		return std::hash<::vxldollar::public_key> () (value_a);
	}
};
template <>
struct hash<::vxldollar::uint512_union>
{
	size_t operator() (::vxldollar::uint512_union const & value_a) const
	{
		return std::hash<::vxldollar::uint512_union> () (value_a);
	}
};
template <>
struct hash<::vxldollar::qualified_root>
{
	size_t operator() (::vxldollar::qualified_root const & value_a) const
	{
		return std::hash<::vxldollar::qualified_root> () (value_a);
	}
};
template <>
struct hash<::vxldollar::root>
{
	size_t operator() (::vxldollar::root const & value_a) const
	{
		return std::hash<::vxldollar::root> () (value_a);
	}
};
}
namespace vxldollar
{
/**
 * A key pair. The private key is generated from the random pool, or passed in
 * as a hex string. The public key is derived using ed25519.
 */
class keypair
{
public:
	keypair ();
	keypair (std::string const &);
	keypair (vxldollar::raw_key &&);
	vxldollar::public_key pub;
	vxldollar::raw_key prv;
};

/**
 * Latest information about an account
 */
class account_info final
{
public:
	account_info () = default;
	account_info (vxldollar::block_hash const &, vxldollar::account const &, vxldollar::block_hash const &, vxldollar::amount const &, uint64_t, uint64_t, epoch);
	bool deserialize (vxldollar::stream &);
	bool operator== (vxldollar::account_info const &) const;
	bool operator!= (vxldollar::account_info const &) const;
	size_t db_size () const;
	vxldollar::epoch epoch () const;
	vxldollar::block_hash head{ 0 };
	vxldollar::account representative{};
	vxldollar::block_hash open_block{ 0 };
	vxldollar::amount balance{ 0 };
	/** Seconds since posix epoch */
	uint64_t modified{ 0 };
	uint64_t block_count{ 0 };
	vxldollar::epoch epoch_m{ vxldollar::epoch::epoch_0 };
};

/**
 * Information on an uncollected send
 */
class pending_info final
{
public:
	pending_info () = default;
	pending_info (vxldollar::account const &, vxldollar::amount const &, vxldollar::epoch);
	size_t db_size () const;
	bool deserialize (vxldollar::stream &);
	bool operator== (vxldollar::pending_info const &) const;
	vxldollar::account source{};
	vxldollar::amount amount{ 0 };
	vxldollar::epoch epoch{ vxldollar::epoch::epoch_0 };
};
class pending_key final
{
public:
	pending_key () = default;
	pending_key (vxldollar::account const &, vxldollar::block_hash const &);
	bool deserialize (vxldollar::stream &);
	bool operator== (vxldollar::pending_key const &) const;
	vxldollar::account const & key () const;
	vxldollar::account account{};
	vxldollar::block_hash hash{ 0 };
};

class endpoint_key final
{
public:
	endpoint_key () = default;

	/*
     * @param address_a This should be in network byte order
     * @param port_a This should be in host byte order
     */
	endpoint_key (std::array<uint8_t, 16> const & address_a, uint16_t port_a);

	/*
     * @return The ipv6 address in network byte order
     */
	std::array<uint8_t, 16> const & address_bytes () const;

	/*
     * @return The port in host byte order
     */
	uint16_t port () const;

private:
	// Both stored internally in network byte order
	std::array<uint8_t, 16> address;
	uint16_t network_port{ 0 };
};

enum class no_value
{
	dummy
};

class unchecked_key final
{
public:
	unchecked_key () = default;
	explicit unchecked_key (vxldollar::hash_or_account const & dependency);
	unchecked_key (vxldollar::hash_or_account const &, vxldollar::block_hash const &);
	unchecked_key (vxldollar::uint512_union const &);
	bool deserialize (vxldollar::stream &);
	bool operator== (vxldollar::unchecked_key const &) const;
	vxldollar::block_hash const & key () const;
	vxldollar::block_hash previous{ 0 };
	vxldollar::block_hash hash{ 0 };
};

/**
 * Tag for block signature verification result
 */
enum class signature_verification : uint8_t
{
	unknown = 0,
	invalid = 1,
	valid = 2,
	valid_epoch = 3 // Valid for epoch blocks
};

/**
 * Information on an unchecked block
 */
class unchecked_info final
{
public:
	unchecked_info () = default;
	unchecked_info (std::shared_ptr<vxldollar::block> const &, vxldollar::account const &, vxldollar::signature_verification = vxldollar::signature_verification::unknown);
	unchecked_info (std::shared_ptr<vxldollar::block> const &);
	void serialize (vxldollar::stream &) const;
	bool deserialize (vxldollar::stream &);
	uint64_t modified () const;
	std::shared_ptr<vxldollar::block> block;
	vxldollar::account account{};

private:
	/** Seconds since posix epoch */
	uint64_t modified_m{ 0 };

public:
	vxldollar::signature_verification verified{ vxldollar::signature_verification::unknown };
};

class block_info final
{
public:
	block_info () = default;
	block_info (vxldollar::account const &, vxldollar::amount const &);
	vxldollar::account account{};
	vxldollar::amount balance{ 0 };
};

class confirmation_height_info final
{
public:
	confirmation_height_info () = default;
	confirmation_height_info (uint64_t, vxldollar::block_hash const &);

	void serialize (vxldollar::stream &) const;
	bool deserialize (vxldollar::stream &);

	/** height of the cemented frontier */
	uint64_t height{};

	/** hash of the highest cemented block, the cemented/confirmed frontier */
	vxldollar::block_hash frontier{};
};

namespace confirmation_height
{
	/** When the uncemented count (block count - cemented count) is less than this use the unbounded processor */
	uint64_t const unbounded_cutoff{ 16384 };
}

using vote_blocks_vec_iter = std::vector<boost::variant<std::shared_ptr<vxldollar::block>, vxldollar::block_hash>>::const_iterator;
class iterate_vote_blocks_as_hash final
{
public:
	iterate_vote_blocks_as_hash () = default;
	vxldollar::block_hash operator() (boost::variant<std::shared_ptr<vxldollar::block>, vxldollar::block_hash> const & item) const;
};
class vote final
{
public:
	vote () = default;
	vote (vxldollar::vote const &);
	vote (bool &, vxldollar::stream &, vxldollar::block_uniquer * = nullptr);
	vote (bool &, vxldollar::stream &, vxldollar::block_type, vxldollar::block_uniquer * = nullptr);
	vote (vxldollar::account const &, vxldollar::raw_key const &, uint64_t timestamp, uint8_t duration, std::shared_ptr<vxldollar::block> const &);
	vote (vxldollar::account const &, vxldollar::raw_key const &, uint64_t timestamp, uint8_t duration, std::vector<vxldollar::block_hash> const &);
	std::string hashes_string () const;
	vxldollar::block_hash hash () const;
	vxldollar::block_hash full_hash () const;
	bool operator== (vxldollar::vote const &) const;
	bool operator!= (vxldollar::vote const &) const;
	void serialize (vxldollar::stream &, vxldollar::block_type) const;
	void serialize (vxldollar::stream &) const;
	void serialize_json (boost::property_tree::ptree & tree) const;
	bool deserialize (vxldollar::stream &, vxldollar::block_uniquer * = nullptr);
	bool validate () const;
	boost::transform_iterator<vxldollar::iterate_vote_blocks_as_hash, vxldollar::vote_blocks_vec_iter> begin () const;
	boost::transform_iterator<vxldollar::iterate_vote_blocks_as_hash, vxldollar::vote_blocks_vec_iter> end () const;
	std::string to_json () const;
	uint64_t timestamp () const;
	uint8_t duration_bits () const;
	std::chrono::milliseconds duration () const;
	static uint64_t constexpr timestamp_mask = { 0xffff'ffff'ffff'fff0ULL };
	static uint64_t constexpr timestamp_max = { 0xffff'ffff'ffff'fff0ULL };
	static uint64_t constexpr timestamp_min = { 0x0000'0000'0000'0010ULL };
	static uint8_t constexpr duration_max = { 0x0fu };

private:
	// Vote timestamp
	uint64_t timestamp_m;

public:
	// The blocks, or block hashes, that this vote is for
	std::vector<boost::variant<std::shared_ptr<vxldollar::block>, vxldollar::block_hash>> blocks;
	// Account that's voting
	vxldollar::account account;
	// Signature of timestamp + block hashes
	vxldollar::signature signature;
	static std::string const hash_prefix;

private:
	uint64_t packed_timestamp (uint64_t timestamp, uint8_t duration) const;
};
/**
 * This class serves to find and return unique variants of a vote in order to minimize memory usage
 */
class vote_uniquer final
{
public:
	using value_type = std::pair<vxldollar::block_hash const, std::weak_ptr<vxldollar::vote>>;

	vote_uniquer (vxldollar::block_uniquer &);
	std::shared_ptr<vxldollar::vote> unique (std::shared_ptr<vxldollar::vote> const &);
	size_t size ();

private:
	vxldollar::block_uniquer & uniquer;
	vxldollar::mutex mutex{ mutex_identifier (mutexes::vote_uniquer) };
	std::unordered_map<std::remove_const_t<value_type::first_type>, value_type::second_type> votes;
	static unsigned constexpr cleanup_count = 2;
};

std::unique_ptr<container_info_component> collect_container_info (vote_uniquer & vote_uniquer, std::string const & name);

enum class vote_code
{
	invalid, // Vote is not signed correctly
	replay, // Vote does not have the highest timestamp, it's a replay
	vote, // Vote has the highest timestamp
	indeterminate // Unknown if replay or vote
};

enum class process_result
{
	progress, // Hasn't been seen before, signed correctly
	bad_signature, // Signature was bad, forged or transmission error
	old, // Already seen and was valid
	negative_spend, // Malicious attempt to spend a negative amount
	fork, // Malicious fork based on previous
	unreceivable, // Source block doesn't exist, has already been received, or requires an account upgrade (epoch blocks)
	gap_previous, // Block marked as previous is unknown
	gap_source, // Block marked as source is unknown
	gap_epoch_open_pending, // Block marked as pending blocks required for epoch open block are unknown
	opened_burn_account, // The impossible happened, someone found the private key associated with the public key '0'.
	balance_mismatch, // Balance and amount delta don't match
	representative_mismatch, // Representative is changed when it is not allowed
	block_position, // This block cannot follow the previous block
	insufficient_work // Insufficient work for this block, even though it passed the minimal validation
};
class process_return final
{
public:
	vxldollar::process_result code;
	vxldollar::signature_verification verified;
	vxldollar::amount previous_balance;
};
enum class tally_result
{
	vote,
	changed,
	confirm
};

class network_params;

/** Genesis keys and ledger constants for network variants */
class ledger_constants
{
public:
	ledger_constants (vxldollar::work_thresholds & work, vxldollar::networks network_a);
	vxldollar::work_thresholds & work;
	vxldollar::keypair zero_key;
	vxldollar::account vxldollar_beta_account;
	vxldollar::account vxldollar_live_account;
	vxldollar::account vxldollar_test_account;
	std::shared_ptr<vxldollar::block> vxldollar_dev_genesis;
	std::shared_ptr<vxldollar::block> vxldollar_beta_genesis;
	std::shared_ptr<vxldollar::block> vxldollar_live_genesis;
	std::shared_ptr<vxldollar::block> vxldollar_test_genesis;
	std::shared_ptr<vxldollar::block> genesis;
	vxldollar::uint128_t genesis_amount;
	vxldollar::account burn_account;
	vxldollar::account vxldollar_dev_final_votes_canary_account;
	vxldollar::account vxldollar_beta_final_votes_canary_account;
	vxldollar::account vxldollar_live_final_votes_canary_account;
	vxldollar::account vxldollar_test_final_votes_canary_account;
	vxldollar::account final_votes_canary_account;
	uint64_t vxldollar_dev_final_votes_canary_height;
	uint64_t vxldollar_beta_final_votes_canary_height;
	uint64_t vxldollar_live_final_votes_canary_height;
	uint64_t vxldollar_test_final_votes_canary_height;
	uint64_t final_votes_canary_height;
	vxldollar::epochs epochs;
};

namespace dev
{
	extern vxldollar::keypair genesis_key;
	extern vxldollar::network_params network_params;
	extern vxldollar::ledger_constants & constants;
	extern std::shared_ptr<vxldollar::block> & genesis;
}

/** Constants which depend on random values (always used as singleton) */
class hardened_constants
{
public:
	static hardened_constants & get ();

	vxldollar::account not_an_account;
	vxldollar::uint128_union random_128;

private:
	hardened_constants ();
};

/** Node related constants whose value depends on the active network */
class node_constants
{
public:
	node_constants (vxldollar::network_constants & network_constants);
	std::chrono::minutes backup_interval;
	std::chrono::seconds search_pending_interval;
	std::chrono::minutes unchecked_cleaning_interval;
	std::chrono::milliseconds process_confirmed_interval;

	/** The maximum amount of samples for a 2 week period on live or 1 day on beta */
	uint64_t max_weight_samples;
	uint64_t weight_period;
};

/** Voting related constants whose value depends on the active network */
class voting_constants
{
public:
	voting_constants (vxldollar::network_constants & network_constants);
	size_t const max_cache;
	std::chrono::seconds const delay;
};

/** Port-mapping related constants whose value depends on the active network */
class portmapping_constants
{
public:
	portmapping_constants (vxldollar::network_constants & network_constants);
	// Timeouts are primes so they infrequently happen at the same time
	std::chrono::seconds lease_duration;
	std::chrono::seconds health_check_period;
};

/** Bootstrap related constants whose value depends on the active network */
class bootstrap_constants
{
public:
	bootstrap_constants (vxldollar::network_constants & network_constants);
	uint32_t lazy_max_pull_blocks;
	uint32_t lazy_min_pull_blocks;
	unsigned frontier_retry_limit;
	unsigned lazy_retry_limit;
	unsigned lazy_destinations_retry_limit;
	std::chrono::milliseconds gap_cache_bootstrap_start_interval;
	uint32_t default_frontiers_age_seconds;
};

/** Constants whose value depends on the active network */
class network_params
{
public:
	/** Populate values based on \p network_a */
	network_params (vxldollar::networks network_a);

	unsigned kdf_work;
	vxldollar::work_thresholds work;
	vxldollar::network_constants network;
	vxldollar::ledger_constants ledger;
	vxldollar::voting_constants voting;
	vxldollar::node_constants node;
	vxldollar::portmapping_constants portmapping;
	vxldollar::bootstrap_constants bootstrap;
};

enum class confirmation_height_mode
{
	automatic,
	unbounded,
	bounded
};

/* Holds flags for various cacheable data. For most CLI operations caching is unnecessary
     * (e.g getting the cemented block count) so it can be disabled for performance reasons. */
class generate_cache
{
public:
	bool reps = true;
	bool cemented_count = true;
	bool unchecked_count = true;
	bool account_count = true;
	bool block_count = true;

	void enable_all ();
};

/* Holds an in-memory cache of various counts */
class ledger_cache
{
public:
	vxldollar::rep_weights rep_weights;
	std::atomic<uint64_t> cemented_count{ 0 };
	std::atomic<uint64_t> block_count{ 0 };
	std::atomic<uint64_t> pruned_count{ 0 };
	std::atomic<uint64_t> account_count{ 0 };
	std::atomic<bool> final_votes_confirmation_canary{ false };
};

/* Defines the possible states for an election to stop in */
enum class election_status_type : uint8_t
{
	ongoing = 0,
	active_confirmed_quorum = 1,
	active_confirmation_height = 2,
	inactive_confirmation_height = 3,
	stopped = 5
};

/* Holds a summary of an election */
class election_status final
{
public:
	std::shared_ptr<vxldollar::block> winner;
	vxldollar::amount tally;
	vxldollar::amount final_tally;
	std::chrono::milliseconds election_end;
	std::chrono::milliseconds election_duration;
	unsigned confirmation_request_count;
	unsigned block_count;
	unsigned voter_count;
	election_status_type type;
};

vxldollar::wallet_id random_wallet_id ();
}
