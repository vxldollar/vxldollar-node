#include "gtest/gtest.h"

#include <vxldollar/node/common.hpp>
#include <vxldollar/node/logging.hpp>
#include <vxldollar/secure/utility.hpp>

#include <boost/filesystem/path.hpp>

namespace vxldollar
{
void cleanup_dev_directories_on_exit ();
void force_vxldollar_dev_network ();
}

GTEST_API_ int main (int argc, char ** argv)
{
	printf ("Running main() from core_test_main.cc\n");
	vxldollar::force_vxldollar_dev_network ();
	vxldollar::node_singleton_memory_pool_purge_guard memory_pool_cleanup_guard;
	// Setting up logging so that there aren't any piped to standard output.
	vxldollar::logging logging;
	logging.init (vxldollar::unique_path ());
	testing::InitGoogleTest (&argc, argv);
	auto res = RUN_ALL_TESTS ();
	vxldollar::cleanup_dev_directories_on_exit ();
	return res;
}
