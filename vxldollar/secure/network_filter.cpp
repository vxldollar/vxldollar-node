#include <vxldollar/crypto_lib/random_pool.hpp>
#include <vxldollar/lib/locks.hpp>
#include <vxldollar/secure/buffer.hpp>
#include <vxldollar/secure/common.hpp>
#include <vxldollar/secure/network_filter.hpp>

vxldollar::network_filter::network_filter (size_t size_a) :
	items (size_a, vxldollar::uint128_t{ 0 })
{
	vxldollar::random_pool::generate_block (key, key.size ());
}

bool vxldollar::network_filter::apply (uint8_t const * bytes_a, size_t count_a, vxldollar::uint128_t * digest_a)
{
	// Get hash before locking
	auto digest (hash (bytes_a, count_a));

	vxldollar::lock_guard<vxldollar::mutex> lock (mutex);
	auto & element (get_element (digest));
	bool existed (element == digest);
	if (!existed)
	{
		// Replace likely old element with a new one
		element = digest;
	}
	if (digest_a)
	{
		*digest_a = digest;
	}
	return existed;
}

void vxldollar::network_filter::clear (vxldollar::uint128_t const & digest_a)
{
	vxldollar::lock_guard<vxldollar::mutex> lock (mutex);
	auto & element (get_element (digest_a));
	if (element == digest_a)
	{
		element = vxldollar::uint128_t{ 0 };
	}
}

void vxldollar::network_filter::clear (std::vector<vxldollar::uint128_t> const & digests_a)
{
	vxldollar::lock_guard<vxldollar::mutex> lock (mutex);
	for (auto const & digest : digests_a)
	{
		auto & element (get_element (digest));
		if (element == digest)
		{
			element = vxldollar::uint128_t{ 0 };
		}
	}
}

void vxldollar::network_filter::clear (uint8_t const * bytes_a, size_t count_a)
{
	clear (hash (bytes_a, count_a));
}

template <typename OBJECT>
void vxldollar::network_filter::clear (OBJECT const & object_a)
{
	clear (hash (object_a));
}

void vxldollar::network_filter::clear ()
{
	vxldollar::lock_guard<vxldollar::mutex> lock (mutex);
	items.assign (items.size (), vxldollar::uint128_t{ 0 });
}

template <typename OBJECT>
vxldollar::uint128_t vxldollar::network_filter::hash (OBJECT const & object_a) const
{
	std::vector<uint8_t> bytes;
	{
		vxldollar::vectorstream stream (bytes);
		object_a->serialize (stream);
	}
	return hash (bytes.data (), bytes.size ());
}

vxldollar::uint128_t & vxldollar::network_filter::get_element (vxldollar::uint128_t const & hash_a)
{
	debug_assert (!mutex.try_lock ());
	debug_assert (items.size () > 0);
	size_t index (hash_a % items.size ());
	return items[index];
}

vxldollar::uint128_t vxldollar::network_filter::hash (uint8_t const * bytes_a, size_t count_a) const
{
	vxldollar::uint128_union digest{ 0 };
	siphash_t siphash (key, static_cast<unsigned int> (key.size ()));
	siphash.CalculateDigest (digest.bytes.data (), bytes_a, count_a);
	return digest.number ();
}

// Explicitly instantiate
template vxldollar::uint128_t vxldollar::network_filter::hash (std::shared_ptr<vxldollar::block> const &) const;
template void vxldollar::network_filter::clear (std::shared_ptr<vxldollar::block> const &);
