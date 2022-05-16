#include <vxldollar/lib/config.hpp>
#include <vxldollar/secure/utility.hpp>
#include <vxldollar/secure/working.hpp>

#include <boost/filesystem.hpp>

static std::vector<boost::filesystem::path> all_unique_paths;

boost::filesystem::path vxldollar::working_path (vxldollar::networks network)
{
	auto result (vxldollar::app_path ());
	switch (network)
	{
		case vxldollar::networks::invalid:
			release_assert (false);
			break;
		case vxldollar::networks::vxldollar_dev_network:
			result /= "VxldollarDev";
			break;
		case vxldollar::networks::vxldollar_beta_network:
			result /= "VxldollarBeta";
			break;
		case vxldollar::networks::vxldollar_live_network:
			result /= "Vxldollar";
			break;
		case vxldollar::networks::vxldollar_test_network:
			result /= "VxldollarTest";
			break;
	}
	return result;
}

boost::filesystem::path vxldollar::unique_path (vxldollar::networks network)
{
	auto result (working_path (network) / boost::filesystem::unique_path ());
	all_unique_paths.push_back (result);
	return result;
}

void vxldollar::remove_temporary_directories ()
{
	for (auto & path : all_unique_paths)
	{
		boost::system::error_code ec;
		boost::filesystem::remove_all (path, ec);
		if (ec)
		{
			std::cerr << "Could not remove temporary directory: " << ec.message () << std::endl;
		}

		// lmdb creates a -lock suffixed file for its MDB_NOSUBDIR databases
		auto lockfile = path;
		lockfile += "-lock";
		boost::filesystem::remove (lockfile, ec);
		if (ec)
		{
			std::cerr << "Could not remove temporary lock file: " << ec.message () << std::endl;
		}
	}
}

namespace vxldollar
{
/** A wrapper for handling signals */
std::function<void ()> signal_handler_impl;
void signal_handler (int sig)
{
	if (signal_handler_impl != nullptr)
	{
		signal_handler_impl ();
	}
}
}
