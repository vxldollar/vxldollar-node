#pragma once

#include <vxldollar/lib/errors.hpp>
#include <vxldollar/node/node.hpp>

#include <chrono>

namespace vxldollar
{
/** Test-system related error codes */
enum class error_system
{
	generic = 1,
	deadline_expired
};
class system final
{
public:
	system ();
	system (uint16_t, vxldollar::transport::transport_type = vxldollar::transport::transport_type::tcp, vxldollar::node_flags = vxldollar::node_flags ());
	~system ();
	void ledger_initialization_set (std::vector<vxldollar::keypair> const & reps, vxldollar::amount const & reserve = 0);
	void generate_activity (vxldollar::node &, std::vector<vxldollar::account> &);
	void generate_mass_activity (uint32_t, vxldollar::node &);
	void generate_usage_traffic (uint32_t, uint32_t, size_t);
	void generate_usage_traffic (uint32_t, uint32_t);
	vxldollar::account get_random_account (std::vector<vxldollar::account> &);
	vxldollar::uint128_t get_random_amount (vxldollar::transaction const &, vxldollar::node &, vxldollar::account const &);
	void generate_rollback (vxldollar::node &, std::vector<vxldollar::account> &);
	void generate_change_known (vxldollar::node &, std::vector<vxldollar::account> &);
	void generate_change_unknown (vxldollar::node &, std::vector<vxldollar::account> &);
	void generate_receive (vxldollar::node &);
	void generate_send_new (vxldollar::node &, std::vector<vxldollar::account> &);
	void generate_send_existing (vxldollar::node &, std::vector<vxldollar::account> &);
	std::unique_ptr<vxldollar::state_block> upgrade_genesis_epoch (vxldollar::node &, vxldollar::epoch const);
	std::shared_ptr<vxldollar::wallet> wallet (size_t);
	vxldollar::account account (vxldollar::transaction const &, size_t);
	/** Generate work with difficulty between \p min_difficulty_a (inclusive) and \p max_difficulty_a (exclusive) */
	uint64_t work_generate_limited (vxldollar::block_hash const & root_a, uint64_t min_difficulty_a, uint64_t max_difficulty_a);
	/**
	 * Polls, sleep if there's no work to be done (default 50ms), then check the deadline
	 * @returns 0 or vxldollar::deadline_expired
	 */
	std::error_code poll (std::chrono::nanoseconds const & sleep_time = std::chrono::milliseconds (50));
	std::error_code poll_until_true (std::chrono::nanoseconds deadline, std::function<bool ()>);
	void delay_ms (std::chrono::milliseconds const & delay);
	void stop ();
	void deadline_set (std::chrono::duration<double, std::nano> const & delta);
	std::shared_ptr<vxldollar::node> add_node (vxldollar::node_flags = vxldollar::node_flags (), vxldollar::transport::transport_type = vxldollar::transport::transport_type::tcp);
	std::shared_ptr<vxldollar::node> add_node (vxldollar::node_config const &, vxldollar::node_flags = vxldollar::node_flags (), vxldollar::transport::transport_type = vxldollar::transport::transport_type::tcp);
	boost::asio::io_context io_ctx;
	std::vector<std::shared_ptr<vxldollar::node>> nodes;
	vxldollar::logging logging;
	vxldollar::work_pool work{ vxldollar::dev::network_params.network, std::max (std::thread::hardware_concurrency (), 1u) };
	std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<double>> deadline{ std::chrono::steady_clock::time_point::max () };
	double deadline_scaling_factor{ 1.0 };
	unsigned node_sequence{ 0 };
	std::vector<std::shared_ptr<vxldollar::block>> initialization_blocks;
};
std::unique_ptr<vxldollar::state_block> upgrade_epoch (vxldollar::work_pool &, vxldollar::ledger &, vxldollar::epoch);
void blocks_confirm (vxldollar::node &, std::vector<std::shared_ptr<vxldollar::block>> const &, bool const = false);
uint16_t get_available_port ();
void cleanup_dev_directories_on_exit ();
}
REGISTER_ERROR_CODES (vxldollar, error_system);
