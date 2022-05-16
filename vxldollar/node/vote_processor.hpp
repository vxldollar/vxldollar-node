#pragma once

#include <vxldollar/lib/numbers.hpp>
#include <vxldollar/lib/utility.hpp>
#include <vxldollar/secure/common.hpp>

#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_set>

namespace vxldollar
{
class signature_checker;
class active_transactions;
class store;
class node_observers;
class stats;
class node_config;
class logger_mt;
class online_reps;
class rep_crawler;
class ledger;
class network_params;
class node_flags;
class stat;

class transaction;
namespace transport
{
	class channel;
}

class vote_processor final
{
public:
	explicit vote_processor (vxldollar::signature_checker & checker_a, vxldollar::active_transactions & active_a, vxldollar::node_observers & observers_a, vxldollar::stat & stats_a, vxldollar::node_config & config_a, vxldollar::node_flags & flags_a, vxldollar::logger_mt & logger_a, vxldollar::online_reps & online_reps_a, vxldollar::rep_crawler & rep_crawler_a, vxldollar::ledger & ledger_a, vxldollar::network_params & network_params_a);
	/** Returns false if the vote was processed */
	bool vote (std::shared_ptr<vxldollar::vote> const &, std::shared_ptr<vxldollar::transport::channel> const &);
	/** Note: node.active.mutex lock is required */
	vxldollar::vote_code vote_blocking (std::shared_ptr<vxldollar::vote> const &, std::shared_ptr<vxldollar::transport::channel> const &, bool = false);
	void verify_votes (std::deque<std::pair<std::shared_ptr<vxldollar::vote>, std::shared_ptr<vxldollar::transport::channel>>> const &);
	void flush ();
	/** Block until the currently active processing cycle finishes */
	void flush_active ();
	std::size_t size ();
	bool empty ();
	bool half_full ();
	void calculate_weights ();
	void stop ();
	std::atomic<uint64_t> total_processed{ 0 };

private:
	void process_loop ();

	vxldollar::signature_checker & checker;
	vxldollar::active_transactions & active;
	vxldollar::node_observers & observers;
	vxldollar::stat & stats;
	vxldollar::node_config & config;
	vxldollar::logger_mt & logger;
	vxldollar::online_reps & online_reps;
	vxldollar::rep_crawler & rep_crawler;
	vxldollar::ledger & ledger;
	vxldollar::network_params & network_params;
	std::size_t max_votes;
	std::deque<std::pair<std::shared_ptr<vxldollar::vote>, std::shared_ptr<vxldollar::transport::channel>>> votes;
	/** Representatives levels for random early detection */
	std::unordered_set<vxldollar::account> representatives_1;
	std::unordered_set<vxldollar::account> representatives_2;
	std::unordered_set<vxldollar::account> representatives_3;
	vxldollar::condition_variable condition;
	vxldollar::mutex mutex{ mutex_identifier (mutexes::vote_processor) };
	bool started;
	bool stopped;
	bool is_active;
	std::thread thread;

	friend std::unique_ptr<container_info_component> collect_container_info (vote_processor & vote_processor, std::string const & name);
	friend class vote_processor_weights_Test;
};

std::unique_ptr<container_info_component> collect_container_info (vote_processor & vote_processor, std::string const & name);
}
