#include <vxldollar/lib/epoch.hpp>
#include <vxldollar/secure/common.hpp>

#include <gtest/gtest.h>

TEST (epochs, is_epoch_link)
{
	vxldollar::epochs epochs;
	// Test epoch 1
	vxldollar::keypair key1;
	auto link1 = 42;
	auto link2 = 43;
	ASSERT_FALSE (epochs.is_epoch_link (link1));
	ASSERT_FALSE (epochs.is_epoch_link (link2));
	epochs.add (vxldollar::epoch::epoch_1, key1.pub, link1);
	ASSERT_TRUE (epochs.is_epoch_link (link1));
	ASSERT_FALSE (epochs.is_epoch_link (link2));
	ASSERT_EQ (key1.pub, epochs.signer (vxldollar::epoch::epoch_1));
	ASSERT_EQ (epochs.epoch (link1), vxldollar::epoch::epoch_1);

	// Test epoch 2
	vxldollar::keypair key2;
	epochs.add (vxldollar::epoch::epoch_2, key2.pub, link2);
	ASSERT_TRUE (epochs.is_epoch_link (link2));
	ASSERT_EQ (key2.pub, epochs.signer (vxldollar::epoch::epoch_2));
	ASSERT_EQ (vxldollar::uint256_union (link1), epochs.link (vxldollar::epoch::epoch_1));
	ASSERT_EQ (vxldollar::uint256_union (link2), epochs.link (vxldollar::epoch::epoch_2));
	ASSERT_EQ (epochs.epoch (link2), vxldollar::epoch::epoch_2);
}

TEST (epochs, is_sequential)
{
	ASSERT_TRUE (vxldollar::epochs::is_sequential (vxldollar::epoch::epoch_0, vxldollar::epoch::epoch_1));
	ASSERT_TRUE (vxldollar::epochs::is_sequential (vxldollar::epoch::epoch_1, vxldollar::epoch::epoch_2));

	ASSERT_FALSE (vxldollar::epochs::is_sequential (vxldollar::epoch::epoch_0, vxldollar::epoch::epoch_2));
	ASSERT_FALSE (vxldollar::epochs::is_sequential (vxldollar::epoch::epoch_0, vxldollar::epoch::invalid));
	ASSERT_FALSE (vxldollar::epochs::is_sequential (vxldollar::epoch::unspecified, vxldollar::epoch::epoch_1));
	ASSERT_FALSE (vxldollar::epochs::is_sequential (vxldollar::epoch::epoch_1, vxldollar::epoch::epoch_0));
	ASSERT_FALSE (vxldollar::epochs::is_sequential (vxldollar::epoch::epoch_2, vxldollar::epoch::epoch_0));
	ASSERT_FALSE (vxldollar::epochs::is_sequential (vxldollar::epoch::epoch_2, vxldollar::epoch::epoch_2));
}
