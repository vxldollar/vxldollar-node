#include <vxldollar/node/prioritization.hpp>
#include <vxldollar/secure/common.hpp>

#include <gtest/gtest.h>

#include <unordered_set>

vxldollar::keypair & keyzero ()
{
	static vxldollar::keypair result;
	return result;
}
vxldollar::keypair & key0 ()
{
	static vxldollar::keypair result;
	return result;
}
vxldollar::keypair & key1 ()
{
	static vxldollar::keypair result;
	return result;
}
vxldollar::keypair & key2 ()
{
	static vxldollar::keypair result;
	return result;
}
vxldollar::keypair & key3 ()
{
	static vxldollar::keypair result;
	return result;
}
std::shared_ptr<vxldollar::state_block> & blockzero ()
{
	static std::shared_ptr<vxldollar::state_block> result = std::make_shared<vxldollar::state_block> (keyzero ().pub, 0, keyzero ().pub, 0, 0, keyzero ().prv, keyzero ().pub, 0);
	return result;
}
std::shared_ptr<vxldollar::state_block> & block0 ()
{
	static std::shared_ptr<vxldollar::state_block> result = std::make_shared<vxldollar::state_block> (key0 ().pub, 0, key0 ().pub, vxldollar::Gxrb_ratio, 0, key0 ().prv, key0 ().pub, 0);
	return result;
}
std::shared_ptr<vxldollar::state_block> & block1 ()
{
	static std::shared_ptr<vxldollar::state_block> result = std::make_shared<vxldollar::state_block> (key1 ().pub, 0, key1 ().pub, vxldollar::Mxrb_ratio, 0, key1 ().prv, key1 ().pub, 0);
	return result;
}
std::shared_ptr<vxldollar::state_block> & block2 ()
{
	static std::shared_ptr<vxldollar::state_block> result = std::make_shared<vxldollar::state_block> (key2 ().pub, 0, key2 ().pub, vxldollar::Gxrb_ratio, 0, key2 ().prv, key2 ().pub, 0);
	return result;
}
std::shared_ptr<vxldollar::state_block> & block3 ()
{
	static std::shared_ptr<vxldollar::state_block> result = std::make_shared<vxldollar::state_block> (key3 ().pub, 0, key3 ().pub, vxldollar::Mxrb_ratio, 0, key3 ().prv, key3 ().pub, 0);
	return result;
}

TEST (prioritization, construction)
{
	vxldollar::prioritization prioritization;
	ASSERT_EQ (0, prioritization.size ());
	ASSERT_TRUE (prioritization.empty ());
	ASSERT_EQ (129, prioritization.bucket_count ());
}

TEST (prioritization, insert_zero)
{
	vxldollar::prioritization prioritization;
	prioritization.push (1000, block0 ());
	ASSERT_EQ (1, prioritization.size ());
	ASSERT_EQ (1, prioritization.bucket_size (110));
}

TEST (prioritization, insert_one)
{
	vxldollar::prioritization prioritization;
	prioritization.push (1000, block1 ());
	ASSERT_EQ (1, prioritization.size ());
	ASSERT_EQ (1, prioritization.bucket_size (100));
}

TEST (prioritization, insert_same_priority)
{
	vxldollar::prioritization prioritization;
	prioritization.push (1000, block0 ());
	prioritization.push (1000, block2 ());
	ASSERT_EQ (2, prioritization.size ());
	ASSERT_EQ (2, prioritization.bucket_size (110));
}

TEST (prioritization, insert_duplicate)
{
	vxldollar::prioritization prioritization;
	prioritization.push (1000, block0 ());
	prioritization.push (1000, block0 ());
	ASSERT_EQ (1, prioritization.size ());
	ASSERT_EQ (1, prioritization.bucket_size (110));
}

TEST (prioritization, insert_older)
{
	vxldollar::prioritization prioritization;
	prioritization.push (1000, block0 ());
	prioritization.push (1100, block2 ());
	ASSERT_EQ (block0 (), prioritization.top ());
	prioritization.pop ();
	ASSERT_EQ (block2 (), prioritization.top ());
	prioritization.pop ();
}

TEST (prioritization, pop)
{
	vxldollar::prioritization prioritization;
	ASSERT_TRUE (prioritization.empty ());
	prioritization.push (1000, block0 ());
	ASSERT_FALSE (prioritization.empty ());
	prioritization.pop ();
	ASSERT_TRUE (prioritization.empty ());
}

TEST (prioritization, top_one)
{
	vxldollar::prioritization prioritization;
	prioritization.push (1000, block0 ());
	ASSERT_EQ (block0 (), prioritization.top ());
}

TEST (prioritization, top_two)
{
	vxldollar::prioritization prioritization;
	prioritization.push (1000, block0 ());
	prioritization.push (1, block1 ());
	ASSERT_EQ (block0 (), prioritization.top ());
	prioritization.pop ();
	ASSERT_EQ (block1 (), prioritization.top ());
	prioritization.pop ();
	ASSERT_TRUE (prioritization.empty ());
}

TEST (prioritization, top_round_robin)
{
	vxldollar::prioritization prioritization;
	prioritization.push (1000, blockzero ());
	ASSERT_EQ (blockzero (), prioritization.top ());
	prioritization.push (1000, block0 ());
	prioritization.push (1000, block1 ());
	prioritization.push (1100, block3 ());
	prioritization.pop (); // blockzero
	EXPECT_EQ (block1 (), prioritization.top ());
	prioritization.pop ();
	EXPECT_EQ (block0 (), prioritization.top ());
	prioritization.pop ();
	EXPECT_EQ (block3 (), prioritization.top ());
	prioritization.pop ();
	EXPECT_TRUE (prioritization.empty ());
}

TEST (prioritization, trim_normal)
{
	vxldollar::prioritization prioritization{ 1 };
	prioritization.push (1000, block0 ());
	prioritization.push (1100, block2 ());
	ASSERT_EQ (1, prioritization.size ());
	ASSERT_EQ (block0 (), prioritization.top ());
}

TEST (prioritization, trim_reverse)
{
	vxldollar::prioritization prioritization{ 1 };
	prioritization.push (1100, block2 ());
	prioritization.push (1000, block0 ());
	ASSERT_EQ (1, prioritization.size ());
	ASSERT_EQ (block0 (), prioritization.top ());
}

TEST (prioritization, trim_even)
{
	vxldollar::prioritization prioritization{ 2 };
	prioritization.push (1000, block0 ());
	prioritization.push (1100, block2 ());
	ASSERT_EQ (1, prioritization.size ());
	ASSERT_EQ (block0 (), prioritization.top ());
	prioritization.push (1000, block1 ());
	ASSERT_EQ (2, prioritization.size ());
	ASSERT_EQ (block0 (), prioritization.top ());
	prioritization.pop ();
	ASSERT_EQ (block1 (), prioritization.top ());
}
