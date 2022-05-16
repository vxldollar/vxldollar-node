#include <vxldollar/crypto_lib/random_pool.hpp>
#include <vxldollar/node/bootstrap/bootstrap.hpp>
#include <vxldollar/node/bootstrap/bootstrap_attempt.hpp>
#include <vxldollar/node/bootstrap/bootstrap_bulk_push.hpp>
#include <vxldollar/node/bootstrap/bootstrap_frontier.hpp>
#include <vxldollar/node/common.hpp>
#include <vxldollar/node/node.hpp>
#include <vxldollar/node/transport/tcp.hpp>
#include <vxldollar/node/websocket.hpp>

#include <boost/format.hpp>

#include <algorithm>

constexpr unsigned vxldollar::bootstrap_limits::requeued_pulls_limit;
constexpr unsigned vxldollar::bootstrap_limits::requeued_pulls_limit_dev;

vxldollar::bootstrap_attempt::bootstrap_attempt (std::shared_ptr<vxldollar::node> const & node_a, vxldollar::bootstrap_mode mode_a, uint64_t incremental_id_a, std::string id_a) :
	node (node_a),
	incremental_id (incremental_id_a),
	id (id_a),
	mode (mode_a)
{
	if (id.empty ())
	{
		id = vxldollar::hardened_constants::get ().random_128.to_string ();
	}

	node->logger.always_log (boost::str (boost::format ("Starting %1% bootstrap attempt with ID %2%") % mode_text () % id));
	node->bootstrap_initiator.notify_listeners (true);
	if (node->websocket_server)
	{
		vxldollar::websocket::message_builder builder;
		node->websocket_server->broadcast (builder.bootstrap_started (id, mode_text ()));
	}
}

vxldollar::bootstrap_attempt::~bootstrap_attempt ()
{
	node->logger.always_log (boost::str (boost::format ("Exiting %1% bootstrap attempt with ID %2%") % mode_text () % id));
	node->bootstrap_initiator.notify_listeners (false);
	if (node->websocket_server)
	{
		vxldollar::websocket::message_builder builder;
		node->websocket_server->broadcast (builder.bootstrap_exited (id, mode_text (), attempt_start, total_blocks));
	}
}

bool vxldollar::bootstrap_attempt::should_log ()
{
	vxldollar::lock_guard<vxldollar::mutex> guard (next_log_mutex);
	auto result (false);
	auto now (std::chrono::steady_clock::now ());
	if (next_log < now)
	{
		result = true;
		next_log = now + std::chrono::seconds (15);
	}
	return result;
}

bool vxldollar::bootstrap_attempt::still_pulling ()
{
	debug_assert (!mutex.try_lock ());
	auto running (!stopped);
	auto still_pulling (pulling > 0);
	return running && still_pulling;
}

void vxldollar::bootstrap_attempt::pull_started ()
{
	{
		vxldollar::lock_guard<vxldollar::mutex> guard (mutex);
		++pulling;
	}
	condition.notify_all ();
}

void vxldollar::bootstrap_attempt::pull_finished ()
{
	{
		vxldollar::lock_guard<vxldollar::mutex> guard (mutex);
		--pulling;
	}
	condition.notify_all ();
}

void vxldollar::bootstrap_attempt::stop ()
{
	{
		vxldollar::lock_guard<vxldollar::mutex> lock (mutex);
		stopped = true;
	}
	condition.notify_all ();
	node->bootstrap_initiator.connections->clear_pulls (incremental_id);
}

std::string vxldollar::bootstrap_attempt::mode_text ()
{
	std::string mode_text;
	if (mode == vxldollar::bootstrap_mode::legacy)
	{
		mode_text = "legacy";
	}
	else if (mode == vxldollar::bootstrap_mode::lazy)
	{
		mode_text = "lazy";
	}
	else if (mode == vxldollar::bootstrap_mode::wallet_lazy)
	{
		mode_text = "wallet_lazy";
	}
	return mode_text;
}

void vxldollar::bootstrap_attempt::add_frontier (vxldollar::pull_info const &)
{
	debug_assert (mode == vxldollar::bootstrap_mode::legacy);
}

void vxldollar::bootstrap_attempt::add_bulk_push_target (vxldollar::block_hash const &, vxldollar::block_hash const &)
{
	debug_assert (mode == vxldollar::bootstrap_mode::legacy);
}

bool vxldollar::bootstrap_attempt::request_bulk_push_target (std::pair<vxldollar::block_hash, vxldollar::block_hash> &)
{
	debug_assert (mode == vxldollar::bootstrap_mode::legacy);
	return true;
}

void vxldollar::bootstrap_attempt::set_start_account (vxldollar::account const &)
{
	debug_assert (mode == vxldollar::bootstrap_mode::legacy);
}

bool vxldollar::bootstrap_attempt::process_block (std::shared_ptr<vxldollar::block> const & block_a, vxldollar::account const & known_account_a, uint64_t pull_blocks_processed, vxldollar::bulk_pull::count_t max_blocks, bool block_expected, unsigned retry_limit)
{
	bool stop_pull (false);
	// If block already exists in the ledger, then we can avoid next part of long account chain
	if (pull_blocks_processed % vxldollar::bootstrap_limits::pull_count_per_check == 0 && node->ledger.block_or_pruned_exists (block_a->hash ()))
	{
		stop_pull = true;
	}
	else
	{
		vxldollar::unchecked_info info (block_a, known_account_a, vxldollar::signature_verification::unknown);
		node->block_processor.add (info);
	}
	return stop_pull;
}

bool vxldollar::bootstrap_attempt::lazy_start (vxldollar::hash_or_account const &, bool)
{
	debug_assert (mode == vxldollar::bootstrap_mode::lazy);
	return false;
}

void vxldollar::bootstrap_attempt::lazy_add (vxldollar::pull_info const &)
{
	debug_assert (mode == vxldollar::bootstrap_mode::lazy);
}

void vxldollar::bootstrap_attempt::lazy_requeue (vxldollar::block_hash const &, vxldollar::block_hash const &)
{
	debug_assert (mode == vxldollar::bootstrap_mode::lazy);
}

uint32_t vxldollar::bootstrap_attempt::lazy_batch_size ()
{
	debug_assert (mode == vxldollar::bootstrap_mode::lazy);
	return node->network_params.bootstrap.lazy_min_pull_blocks;
}

bool vxldollar::bootstrap_attempt::lazy_processed_or_exists (vxldollar::block_hash const &)
{
	debug_assert (mode == vxldollar::bootstrap_mode::lazy);
	return false;
}

bool vxldollar::bootstrap_attempt::lazy_has_expired () const
{
	debug_assert (mode == vxldollar::bootstrap_mode::lazy);
	return true;
}

void vxldollar::bootstrap_attempt::requeue_pending (vxldollar::account const &)
{
	debug_assert (mode == vxldollar::bootstrap_mode::wallet_lazy);
}

void vxldollar::bootstrap_attempt::wallet_start (std::deque<vxldollar::account> &)
{
	debug_assert (mode == vxldollar::bootstrap_mode::wallet_lazy);
}

std::size_t vxldollar::bootstrap_attempt::wallet_size ()
{
	debug_assert (mode == vxldollar::bootstrap_mode::wallet_lazy);
	return 0;
}
