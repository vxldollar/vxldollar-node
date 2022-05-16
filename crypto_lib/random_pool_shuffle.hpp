#pragma once

#include <vxldollar/crypto_lib/random_pool.hpp>

#include <crypto/cryptopp/osrng.h>

namespace vxldollar
{
template <class Iter>
void random_pool_shuffle (Iter begin, Iter end)
{
	random_pool::get_pool ().Shuffle (begin, end);
}
}
