#include <vxldollar/lib/memory.hpp>
#include <vxldollar/node/common.hpp>

#include <gtest/gtest.h>
namespace vxldollar
{
void cleanup_dev_directories_on_exit ();
void force_vxldollar_dev_network ();
}

int main (int argc, char ** argv)
{
	vxldollar::force_vxldollar_dev_network ();
	vxldollar::set_use_memory_pools (false);
	vxldollar::node_singleton_memory_pool_purge_guard cleanup_guard;
	testing::InitGoogleTest (&argc, argv);
	auto res = RUN_ALL_TESTS ();
	vxldollar::cleanup_dev_directories_on_exit ();
	return res;
}
