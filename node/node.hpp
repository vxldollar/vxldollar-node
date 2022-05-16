#pragma once

#include <vxldollar/lib/config.hpp>
#include <vxldollar/lib/stats.hpp>
#include <vxldollar/lib/work.hpp>
#include <vxldollar/node/active_transactions.hpp>
#include <vxldollar/node/blockprocessor.hpp>
#include <vxldollar/node/bootstrap/bootstrap.hpp>
#include <vxldollar/node/bootstrap/bootstrap_attempt.hpp>
#include <vxldollar/node/bootstrap/bootstrap_server.hpp>
#include <vxldollar/node/confirmation_height_processor.hpp>
#include <vxldollar/node/distributed_work_factory.hpp>
#include <vxldollar/node/election.hpp>
#include <vxldollar/node/election_scheduler.hpp>
#include <vxldollar/node/gap_cache.hpp>
#include <vxldollar/node/network.hpp>
#include <vxldollar/node/node_observers.hpp>
#include <vxldollar/node/nodeconfig.hpp>
#include <vxldollar/node/online_reps.hpp>
#include <vxldollar/node/portmapping.hpp>
#include <vxldollar/node/repcrawler.hpp>
#include <vxldollar/node/request_aggregator.hpp>
#include <vxldollar/node/signatures.hpp>
#include <vxldollar/node/telemetry.hpp>
#include <vxldollar/node/unchecked_map.hpp>
#include <vxldollar/node/vote_processor.hpp>
#include <vxldollar/node/wallet.hpp>
#include <vxldollar/node/write_database_queue.hpp>
#include <vxldollar/secure/ledger.hpp>
#include <vxldollar/secure/utility.hpp>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/program_options.hpp>
#include <boost/thread/latch.hpp>

#include <atomic>
#include <memory>
#include <vector>

namespace vxldollar
{
namespace websocket
{
	class listener;
}
class node;
class telemetry;
class work_pool;
class block_arrival_info final
{
public:
	std::chrono::steady_clock::time_point arrival;
	vxldollar::block_hash hash;
};
// This class tracks blocks that are probably live because they arrived in a UDP packet
// This gives a fairly reliable way to differentiate between blocks being inserted via bootstrap or new, live blocks.
class block_arrival final
{
public:
	// Return `true' to indicated an error if the block has already been inserted
	bool add (vxldollar::block_hash const &);
	bool recent (vxldollar::block_hash const &);
	// clang-format off
	class tag_sequence {};
	class tag_hash {};
	boost::multi_index_container<vxldollar::block_arrival_info,
		boost::multi_index::indexed_by<
			boost::multi_index::sequenced<boost::multi_index::tag<tag_sequence>>,
			boost::multi_index::hashed_unique<boost::multi_index::tag<tag_hash>,
				boost::multi_index::member<vxldollar::block_arrival_info, vxldollar::block_hash, &vxldollar::block_arrival_info::hash>>>>
	arrival;
	// clang-format on
	vxldollar::mutex mutex{ mutex_identifier (mutexes::block_arrival) };
	static std::size_t constexpr arrival_size_min = 8 * 1024;
	static std::chrono::seconds constexpr arrival_time_min = std::chrono::seconds (300);
};

std::unique_ptr<container_info_component> collect_container_info (block_arrival & block_arrival, std::string const & name);

std::unique_ptr<container_info_component> collect_container_info (rep_crawler & rep_crawler, std::string const & name);

class node final : public std::enable_shared_from_this<vxldollar::node>
{
public:
	node (boost::asio::io_context &, uint16_t, boost::filesystem::path const &, vxldollar::logging const &, vxldollar::work_pool &, vxldollar::node_flags = vxldollar::node_flags (), unsigned seq = 0);
	node (boost::asio::io_context &, boost::filesystem::path const &, vxldollar::node_config const &, vxldollar::work_pool &, vxldollar::node_flags = vxldollar::node_flags (), unsigned seq = 0);
	~node ();
	template <typename T>
	void background (T action_a)
	{
		io_ctx.post (action_a);
	}
	bool copy_with_compaction (boost::filesystem::path const &);
	void keepalive (std::string const &, uint16_t);
	void start ();
	void stop ();
	std::shared_ptr<vxldollar::node> shared ();
	int store_version ();
	void receive_confirmed (vxldollar::transaction const & block_transaction_a, vxldollar::block_hash const & hash_a, vxldollar::account const & destination_a);
	void process_confirmed_data (vxldollar::transaction const &, std::shared_ptr<vxldollar::block> const &, vxldollar::block_hash const &, vxldollar::account &, vxldollar::uint128_t &, bool &, bool &, vxldollar::account &);
	void process_confirmed (vxldollar::election_status const &, uint64_t = 0);
	void process_active (std::shared_ptr<vxldollar::block> const &);
	[[nodiscard]] vxldollar::process_return process (vxldollar::block &);
	vxldollar::process_return process_local (std::shared_ptr<vxldollar::block> const &);
	void process_local_async (std::shared_ptr<vxldollar::block> const &);
	void keepalive_preconfigured (std::vector<std::string> const &);
	vxldollar::block_hash latest (vxldollar::account const &);
	vxldollar::uint128_t balance (vxldollar::account const &);
	std::shared_ptr<vxldollar::block> block (vxldollar::block_hash const &);
	std::pair<vxldollar::uint128_t, vxldollar::uint128_t> balance_pending (vxldollar::account const &, bool only_confirmed);
	vxldollar::uint128_t weight (vxldollar::account const &);
	vxldollar::block_hash rep_block (vxldollar::account const &);
	vxldollar::uint128_t minimum_principal_weight ();
	vxldollar::uint128_t minimum_principal_weight (vxldollar::uint128_t const &);
	void ongoing_rep_calculation ();
	void ongoing_bootstrap ();
	void ongoing_peer_store ();
	void ongoing_unchecked_cleanup ();
	void ongoing_backlog_population ();
	void backup_wallet ();
	void search_receivable_all ();
	void bootstrap_wallet ();
	void unchecked_cleanup ();
	bool collect_ledger_pruning_targets (std::deque<vxldollar::block_hash> &, vxldollar::account &, uint64_t const, uint64_t const, uint64_t const);
	void ledger_pruning (uint64_t const, bool, bool);
	void ongoing_ledger_pruning ();
	int price (vxldollar::uint128_t const &, int);
	// The default difficulty updates to base only when the first epoch_2 block is processed
	uint64_t default_difficulty (vxldollar::work_version const) const;
	uint64_t default_receive_difficulty (vxldollar::work_version const) const;
	uint64_t max_work_generate_difficulty (vxldollar::work_version const) const;
	bool local_work_generation_enabled () const;
	bool work_generation_enabled () const;
	bool work_generation_enabled (std::vector<std::pair<std::string, uint16_t>> const &) const;
	boost::optional<uint64_t> work_generate_blocking (vxldollar::block &, uint64_t);
	boost::optional<uint64_t> work_generate_blocking (vxldollar::work_version const, vxldollar::root const &, uint64_t, boost::optional<vxldollar::account> const & = boost::none);
	void work_generate (vxldollar::work_version const, vxldollar::root const &, uint64_t, std::function<void (boost::optional<uint64_t>)>, boost::optional<vxldollar::account> const & = boost::none, bool const = false);
	void add_initial_peers ();
	void block_confirm (std::shared_ptr<vxldollar::block> const &);
	bool block_confirmed (vxldollar::block_hash const &);
	bool block_confirmed_or_being_confirmed (vxldollar::transaction const &, vxldollar::block_hash const &);
	void do_rpc_callback (boost::asio::ip::tcp::resolver::iterator i_a, std::string const &, uint16_t, std::shared_ptr<std::string> const &, std::shared_ptr<std::string> const &, std::shared_ptr<boost::asio::ip::tcp::resolver> const &);
	void ongoing_online_weight_calculation ();
	void ongoing_online_weight_calculation_queue ();
	bool online () const;
	bool init_error () const;
	bool epoch_upgrader (vxldollar::raw_key const &, vxldollar::epoch, uint64_t, uint64_t);
	void set_bandwidth_params (std::size_t limit, double ratio);
	std::pair<uint64_t, decltype (vxldollar::ledger::bootstrap_weights)> get_bootstrap_weights () const;
	void populate_backlog ();
	uint64_t get_confirmation_height (vxldollar::transaction const &, vxldollar::account &);
	vxldollar::write_database_queue write_database_queue;
	boost::asio::io_context & io_ctx;
	boost::latch node_initialized_latch;
	vxldollar::node_config config;
	vxldollar::network_params & network_params;
	vxldollar::stat stats;
	vxldollar::thread_pool workers;
	std::shared_ptr<vxldollar::websocket::listener> websocket_server;
	vxldollar::node_flags flags;
	vxldollar::work_pool & work;
	vxldollar::distributed_work_factory distributed_work;
	vxldollar::logger_mt logger;
	std::unique_ptr<vxldollar::store> store_impl;
	vxldollar::store & store;
	vxldollar::unchecked_map unchecked;
	std::unique_ptr<vxldollar::wallets_store> wallets_store_impl;
	vxldollar::wallets_store & wallets_store;
	vxldollar::gap_cache gap_cache;
	vxldollar::ledger ledger;
	vxldollar::signature_checker checker;
	vxldollar::network network;
	std::shared_ptr<vxldollar::telemetry> telemetry;
	vxldollar::bootstrap_initiator bootstrap_initiator;
	vxldollar::bootstrap_listener bootstrap;
	boost::filesystem::path application_path;
	vxldollar::node_observers observers;
	vxldollar::port_mapping port_mapping;
	vxldollar::online_reps online_reps;
	vxldollar::rep_crawler rep_crawler;
	vxldollar::vote_processor vote_processor;
	unsigned warmed_up;
	vxldollar::block_processor block_processor;
	vxldollar::block_arrival block_arrival;
	vxldollar::local_vote_history history;
	vxldollar::keypair node_id;
	vxldollar::block_uniquer block_uniquer;
	vxldollar::vote_uniquer vote_uniquer;
	vxldollar::confirmation_height_processor confirmation_height_processor;
	vxldollar::active_transactions active;
	vxldollar::election_scheduler scheduler;
	vxldollar::request_aggregator aggregator;
	vxldollar::wallets wallets;
	std::chrono::steady_clock::time_point const startup_time;
	std::chrono::seconds unchecked_cutoff = std::chrono::seconds (7 * 24 * 60 * 60); // Week
	std::atomic<bool> unresponsive_work_peers{ false };
	std::atomic<bool> stopped{ false };
	static double constexpr price_max = 16.0;
	static double constexpr free_cutoff = 1024.0;
	// For tests only
	unsigned node_seq;
	// For tests only
	boost::optional<uint64_t> work_generate_blocking (vxldollar::block &);
	// For tests only
	boost::optional<uint64_t> work_generate_blocking (vxldollar::root const &, uint64_t);
	// For tests only
	boost::optional<uint64_t> work_generate_blocking (vxldollar::root const &);

private:
	void long_inactivity_cleanup ();
	void epoch_upgrader_impl (vxldollar::raw_key const &, vxldollar::epoch, uint64_t, uint64_t);
	vxldollar::locked<std::future<void>> epoch_upgrading;
};

std::unique_ptr<container_info_component> collect_container_info (node & node, std::string const & name);

vxldollar::node_flags const & inactive_node_flag_defaults ();

class node_wrapper final
{
public:
	node_wrapper (boost::filesystem::path const & path_a, boost::filesystem::path const & config_path_a, vxldollar::node_flags const & node_flags_a);
	~node_wrapper ();

	vxldollar::network_params network_params;
	std::shared_ptr<boost::asio::io_context> io_context;
	vxldollar::work_pool work;
	std::shared_ptr<vxldollar::node> node;
};

class inactive_node final
{
public:
	inactive_node (boost::filesystem::path const & path_a, vxldollar::node_flags const & node_flags_a);
	inactive_node (boost::filesystem::path const & path_a, boost::filesystem::path const & config_path_a, vxldollar::node_flags const & node_flags_a);

	vxldollar::node_wrapper node_wrapper;
	std::shared_ptr<vxldollar::node> node;
};
std::unique_ptr<vxldollar::inactive_node> default_inactive_node (boost::filesystem::path const &, boost::program_options::variables_map const &);
}
