#include <vxldollar/boost/process/child.hpp>
#include <vxldollar/lib/signal_manager.hpp>
#include <vxldollar/lib/threading.hpp>
#include <vxldollar/lib/tlsconfig.hpp>
#include <vxldollar/lib/utility.hpp>
#include <vxldollar/vxldollar_node/daemon.hpp>
#include <vxldollar/node/cli.hpp>
#include <vxldollar/node/daemonconfig.hpp>
#include <vxldollar/node/ipc/ipc_server.hpp>
#include <vxldollar/node/json_handler.hpp>
#include <vxldollar/node/node.hpp>
#include <vxldollar/node/openclwork.hpp>
#include <vxldollar/rpc/rpc.hpp>

#include <boost/format.hpp>

#include <csignal>
#include <iostream>

namespace
{
void vxldollar_abort_signal_handler (int signum)
{
	// remove `signum` from signal handling when under Windows
#ifdef _WIN32
	std::signal (signum, SIG_DFL);
#endif

	// create some debugging log files
	vxldollar::dump_crash_stacktrace ();
	vxldollar::create_load_memory_address_files ();

	// re-raise signal to call the default handler and exit
	raise (signum);
}

void install_abort_signal_handler ()
{
	// We catch signal SIGSEGV and SIGABRT not via the signal manager because we want these signal handlers
	// to be executed in the stack of the code that caused the signal, so we can dump the stacktrace.
#ifdef _WIN32
	std::signal (SIGSEGV, vxldollar_abort_signal_handler);
	std::signal (SIGABRT, vxldollar_abort_signal_handler);
#else
	struct sigaction sa = {};
	sa.sa_handler = vxldollar_abort_signal_handler;
	sigemptyset (&sa.sa_mask);
	sa.sa_flags = SA_RESETHAND;
	sigaction (SIGSEGV, &sa, NULL);
	sigaction (SIGABRT, &sa, NULL);
#endif
}

volatile sig_atomic_t sig_int_or_term = 0;

constexpr std::size_t OPEN_FILE_DESCRIPTORS_LIMIT = 16384;
}

static void load_and_set_bandwidth_params (std::shared_ptr<vxldollar::node> const & node, boost::filesystem::path const & data_path, vxldollar::node_flags const & flags)
{
	vxldollar::daemon_config config{ data_path, node->network_params };

	auto error = vxldollar::read_node_config_toml (data_path, config, flags.config_overrides);
	if (!error)
	{
		error = vxldollar::flags_config_conflicts (flags, config.node);
		if (!error)
		{
			node->set_bandwidth_params (config.node.bandwidth_limit, config.node.bandwidth_limit_burst_ratio);
		}
	}
}

void vxldollar_daemon::daemon::run (boost::filesystem::path const & data_path, vxldollar::node_flags const & flags)
{
	install_abort_signal_handler ();

	boost::filesystem::create_directories (data_path);
	boost::system::error_code error_chmod;
	vxldollar::set_secure_perm_directory (data_path, error_chmod);
	std::unique_ptr<vxldollar::thread_runner> runner;
	vxldollar::network_params network_params{ vxldollar::network_constants::active_network };
	vxldollar::daemon_config config{ data_path, network_params };
	auto error = vxldollar::read_node_config_toml (data_path, config, flags.config_overrides);
	vxldollar::set_use_memory_pools (config.node.use_memory_pools);
	if (!error)
	{
		error = vxldollar::flags_config_conflicts (flags, config.node);
	}
	if (!error)
	{
		config.node.logging.init (data_path);
		vxldollar::logger_mt logger{ config.node.logging.min_time_between_log_output };

		auto tls_config (std::make_shared<vxldollar::tls_config> ());
		error = vxldollar::read_tls_config_toml (data_path, *tls_config, logger);
		if (error)
		{
			std::cerr << error.get_message () << std::endl;
			std::exit (1);
		}
		else
		{
			config.node.websocket_config.tls_config = tls_config;
		}

		boost::asio::io_context io_ctx;
		auto opencl (vxldollar::opencl_work::create (config.opencl_enable, config.opencl, logger, config.node.network_params.work));
		vxldollar::work_pool opencl_work (config.node.network_params.network, config.node.work_threads, config.node.pow_sleep_interval, opencl ? [&opencl] (vxldollar::work_version const version_a, vxldollar::root const & root_a, uint64_t difficulty_a, std::atomic<int> & ticket_a) {
			return opencl->generate_work (version_a, root_a, difficulty_a, ticket_a);
		}
																																		  : std::function<boost::optional<uint64_t> (vxldollar::work_version const, vxldollar::root const &, uint64_t, std::atomic<int> &)> (nullptr));
		try
		{
			// This avoid a blank prompt during any node initialization delays
			auto initialization_text = "Starting up Vxldollar node...";
			std::cout << initialization_text << std::endl;
			logger.always_log (initialization_text);

			vxldollar::set_file_descriptor_limit (OPEN_FILE_DESCRIPTORS_LIMIT);
			auto const file_descriptor_limit = vxldollar::get_file_descriptor_limit ();
			if (file_descriptor_limit < OPEN_FILE_DESCRIPTORS_LIMIT)
			{
				logger.always_log (boost::format ("WARNING: open file descriptors limit is %1%, lower than the %2% recommended. Node was unable to change it.") % file_descriptor_limit % OPEN_FILE_DESCRIPTORS_LIMIT);
			}
			else
			{
				logger.always_log (boost::format ("Open file descriptors limit is %1%") % file_descriptor_limit);
			}

			// for the daemon start up, if the user hasn't specified a port in
			// the config, we must use the default peering port for the network
			//
			if (!config.node.peering_port.has_value ())
			{
				config.node.peering_port = network_params.network.default_node_port;
			}

			auto node (std::make_shared<vxldollar::node> (io_ctx, data_path, config.node, opencl_work, flags));
			if (!node->init_error ())
			{
				auto network_label = node->network_params.network.get_current_network_as_string ();
				std::time_t dateTime = std::time (nullptr);

				std::cout << "Network: " << network_label << ", version: " << VXLDOLLAR_VERSION_STRING << "\n"
						  << "Path: " << node->application_path.string () << "\n"
						  << "Build Info: " << BUILD_INFO << "\n"
						  << "Database backend: " << node->store.vendor_get () << "\n"
						  << "Start time: " << std::put_time (std::gmtime (&dateTime), "%c UTC") << std::endl;

				auto voting (node->wallets.reps ().voting);
				if (voting > 1)
				{
					std::cout << "Voting with more than one representative can limit performance: " << voting << " representatives are configured" << std::endl;
				}
				node->start ();
				vxldollar::ipc::ipc_server ipc_server (*node, config.rpc);
				std::unique_ptr<boost::process::child> rpc_process;
				std::unique_ptr<boost::process::child> vxldollar_pow_server_process;

				/*if (config.pow_server.enable)
				{
					if (!boost::filesystem::exists (config.pow_server.pow_server_path))
					{
						std::cerr << std::string ("vxldollar_pow_server is configured to start as a child process, however the file cannot be found at: ") + config.pow_server.pow_server_path << std::endl;
						std::exit (1);
					}

					vxldollar_pow_server_process = std::make_unique<boost::process::child> (config.pow_server.pow_server_path, "--config_path", data_path / "config-vxldollar-pow-server.toml");
				}*/

				std::unique_ptr<vxldollar::rpc> rpc;
				std::unique_ptr<vxldollar::rpc_handler_interface> rpc_handler;
				if (config.rpc_enable)
				{
					if (!config.rpc.child_process.enable)
					{
						// Launch rpc in-process
						vxldollar::rpc_config rpc_config{ config.node.network_params.network };
						auto error = vxldollar::read_rpc_config_toml (data_path, rpc_config, flags.rpc_config_overrides);
						if (error)
						{
							std::cout << error.get_message () << std::endl;
							std::exit (1);
						}

						rpc_config.tls_config = tls_config;
						rpc_handler = std::make_unique<vxldollar::inprocess_rpc_handler> (*node, ipc_server, config.rpc, [&ipc_server, &workers = node->workers, &io_ctx] () {
							ipc_server.stop ();
							workers.add_timed_task (std::chrono::steady_clock::now () + std::chrono::seconds (3), [&io_ctx] () {
								io_ctx.stop ();
							});
						});
						rpc = vxldollar::get_rpc (io_ctx, rpc_config, *rpc_handler);
						rpc->start ();
					}
					else
					{
						// Spawn a child rpc process
						if (!boost::filesystem::exists (config.rpc.child_process.rpc_path))
						{
							throw std::runtime_error (std::string ("RPC is configured to spawn a new process however the file cannot be found at: ") + config.rpc.child_process.rpc_path);
						}

						auto network = node->network_params.network.get_current_network_as_string ();
						rpc_process = std::make_unique<boost::process::child> (config.rpc.child_process.rpc_path, "--daemon", "--data_path", data_path, "--network", network);
					}
				}

				debug_assert (!vxldollar::signal_handler_impl);
				vxldollar::signal_handler_impl = [&io_ctx] () {
					io_ctx.stop ();
					sig_int_or_term = 1;
				};

				vxldollar::signal_manager sigman;

				// keep trapping Ctrl-C to avoid a second Ctrl-C interrupting tasks started by the first
				sigman.register_signal_handler (SIGINT, &vxldollar::signal_handler, true);

				// sigterm is less likely to come in bunches so only trap it once
				sigman.register_signal_handler (SIGTERM, &vxldollar::signal_handler, false);

#ifndef _WIN32
				// on sighup we should reload the bandwidth parameters
				std::function<void (int)> sighup_signal_handler ([&node, &data_path, &flags] (int signum) {
					debug_assert (signum == SIGHUP);
					load_and_set_bandwidth_params (node, data_path, flags);
				});
				sigman.register_signal_handler (SIGHUP, sighup_signal_handler, true);
#endif

				runner = std::make_unique<vxldollar::thread_runner> (io_ctx, node->config.io_threads);
				runner->join ();

				if (sig_int_or_term == 1)
				{
					ipc_server.stop ();
					node->stop ();
					if (rpc)
					{
						rpc->stop ();
					}
				}
				if (rpc_process)
				{
					rpc_process->wait ();
				}
			}
			else
			{
				std::cerr << "Error initializing node\n";
			}
		}
		catch (std::runtime_error const & e)
		{
			std::cerr << "Error while running node (" << e.what () << ")\n";
		}
	}
	else
	{
		std::cerr << "Error deserializing config: " << error.get_message () << std::endl;
	}
}
