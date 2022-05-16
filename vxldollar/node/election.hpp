#pragma once

#include <vxldollar/secure/common.hpp>
#include <vxldollar/secure/ledger.hpp>
#include <vxldollar/secure/store.hpp>

#include <atomic>
#include <chrono>
#include <memory>
#include <unordered_set>

namespace vxldollar
{
class channel;
class confirmation_solicitor;
class inactive_cache_information;
class node;
class vote_generator_session;
class vote_info final
{
public:
	std::chrono::steady_clock::time_point time;
	uint64_t timestamp;
	vxldollar::block_hash hash;
};
class vote_with_weight_info final
{
public:
	vxldollar::account representative;
	std::chrono::steady_clock::time_point time;
	uint64_t timestamp;
	vxldollar::block_hash hash;
	vxldollar::uint128_t weight;
};
class election_vote_result final
{
public:
	election_vote_result () = default;
	election_vote_result (bool, bool);
	bool replay{ false };
	bool processed{ false };
};
enum class election_behavior
{
	normal,
	optimistic
};
struct election_extended_status final
{
	vxldollar::election_status status;
	std::unordered_map<vxldollar::account, vxldollar::vote_info> votes;
	vxldollar::tally_t tally;
};
class election final : public std::enable_shared_from_this<vxldollar::election>
{
	// Minimum time between broadcasts of the current winner of an election, as a backup to requesting confirmations
	std::chrono::milliseconds base_latency () const;
	std::function<void (std::shared_ptr<vxldollar::block> const &)> confirmation_action;
	std::function<void (vxldollar::account const &)> live_vote_action;

private: // State management
	enum class state_t
	{
		passive, // only listening for incoming votes
		active, // actively request confirmations
		confirmed, // confirmed but still listening for votes
		expired_confirmed,
		expired_unconfirmed
	};
	static unsigned constexpr passive_duration_factor = 5;
	static unsigned constexpr active_request_count_min = 2;
	static unsigned constexpr confirmed_duration_factor = 5;
	std::atomic<vxldollar::election::state_t> state_m = { state_t::passive };

	static_assert (std::is_trivial<std::chrono::steady_clock::duration> ());
	std::atomic<std::chrono::steady_clock::duration> state_start{ std::chrono::steady_clock::now ().time_since_epoch () };

	// These are modified while not holding the mutex from transition_time only
	std::chrono::steady_clock::time_point last_block = { std::chrono::steady_clock::now () };
	std::chrono::steady_clock::time_point last_req = { std::chrono::steady_clock::time_point () };

	bool valid_change (vxldollar::election::state_t, vxldollar::election::state_t) const;
	bool state_change (vxldollar::election::state_t, vxldollar::election::state_t);

public: // State transitions
	bool transition_time (vxldollar::confirmation_solicitor &);
	void transition_active ();

public: // Status
	bool confirmed () const;
	bool failed () const;
	bool optimistic () const;
	vxldollar::election_extended_status current_status () const;
	std::shared_ptr<vxldollar::block> winner () const;
	std::atomic<unsigned> confirmation_request_count{ 0 };

	void log_votes (vxldollar::tally_t const &, std::string const & = "") const;
	vxldollar::tally_t tally () const;
	bool have_quorum (vxldollar::tally_t const &) const;

	// Guarded by mutex
	vxldollar::election_status status;

public: // Interface
	election (vxldollar::node &, std::shared_ptr<vxldollar::block> const &, std::function<void (std::shared_ptr<vxldollar::block> const &)> const &, std::function<void (vxldollar::account const &)> const &, vxldollar::election_behavior);
	std::shared_ptr<vxldollar::block> find (vxldollar::block_hash const &) const;
	vxldollar::election_vote_result vote (vxldollar::account const &, uint64_t, vxldollar::block_hash const &);
	bool publish (std::shared_ptr<vxldollar::block> const & block_a);
	std::size_t insert_inactive_votes_cache (vxldollar::inactive_cache_information const &);
	// Confirm this block if quorum is met
	void confirm_if_quorum (vxldollar::unique_lock<vxldollar::mutex> &);

public: // Information
	uint64_t const height;
	vxldollar::root const root;
	vxldollar::qualified_root const qualified_root;
	std::vector<vxldollar::vote_with_weight_info> votes_with_weight () const;

private:
	vxldollar::tally_t tally_impl () const;
	// lock_a does not own the mutex on return
	void confirm_once (vxldollar::unique_lock<vxldollar::mutex> & lock_a, vxldollar::election_status_type = vxldollar::election_status_type::active_confirmed_quorum);
	void broadcast_block (vxldollar::confirmation_solicitor &);
	void send_confirm_req (vxldollar::confirmation_solicitor &);
	// Calculate votes for local representatives
	void generate_votes () const;
	void remove_votes (vxldollar::block_hash const &);
	void remove_block (vxldollar::block_hash const &);
	bool replace_by_weight (vxldollar::unique_lock<vxldollar::mutex> & lock_a, vxldollar::block_hash const &);

private:
	std::unordered_map<vxldollar::block_hash, std::shared_ptr<vxldollar::block>> last_blocks;
	std::unordered_map<vxldollar::account, vxldollar::vote_info> last_votes;
	std::atomic<bool> is_quorum{ false };
	mutable vxldollar::uint128_t final_weight{ 0 };
	mutable std::unordered_map<vxldollar::block_hash, vxldollar::uint128_t> last_tally;

	vxldollar::election_behavior const behavior{ vxldollar::election_behavior::normal };
	std::chrono::steady_clock::time_point const election_start = { std::chrono::steady_clock::now () };

	vxldollar::node & node;
	mutable vxldollar::mutex mutex;

	static std::chrono::seconds constexpr late_blocks_delay{ 5 };
	static std::size_t constexpr max_blocks{ 10 };

	friend class active_transactions;
	friend class confirmation_solicitor;

public: // Only used in tests
	void force_confirm (vxldollar::election_status_type = vxldollar::election_status_type::active_confirmed_quorum);
	std::unordered_map<vxldollar::account, vxldollar::vote_info> votes () const;
	std::unordered_map<vxldollar::block_hash, std::shared_ptr<vxldollar::block>> blocks () const;

	friend class confirmation_solicitor_different_hash_Test;
	friend class confirmation_solicitor_bypass_max_requests_cap_Test;
	friend class votes_add_existing_Test;
	friend class votes_add_old_Test;
};
}
