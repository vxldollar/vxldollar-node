#include <vxldollar/lib/rep_weights.hpp>
#include <vxldollar/secure/store.hpp>

void vxldollar::rep_weights::representation_add (vxldollar::account const & source_rep_a, vxldollar::uint128_t const & amount_a)
{
	vxldollar::lock_guard<vxldollar::mutex> guard (mutex);
	auto source_previous (get (source_rep_a));
	put (source_rep_a, source_previous + amount_a);
}

void vxldollar::rep_weights::representation_add_dual (vxldollar::account const & source_rep_1, vxldollar::uint128_t const & amount_1, vxldollar::account const & source_rep_2, vxldollar::uint128_t const & amount_2)
{
	if (source_rep_1 != source_rep_2)
	{
		vxldollar::lock_guard<vxldollar::mutex> guard (mutex);
		auto source_previous_1 (get (source_rep_1));
		put (source_rep_1, source_previous_1 + amount_1);
		auto source_previous_2 (get (source_rep_2));
		put (source_rep_2, source_previous_2 + amount_2);
	}
	else
	{
		representation_add (source_rep_1, amount_1 + amount_2);
	}
}

void vxldollar::rep_weights::representation_put (vxldollar::account const & account_a, vxldollar::uint128_union const & representation_a)
{
	vxldollar::lock_guard<vxldollar::mutex> guard (mutex);
	put (account_a, representation_a);
}

vxldollar::uint128_t vxldollar::rep_weights::representation_get (vxldollar::account const & account_a) const
{
	vxldollar::lock_guard<vxldollar::mutex> lk (mutex);
	return get (account_a);
}

/** Makes a copy */
std::unordered_map<vxldollar::account, vxldollar::uint128_t> vxldollar::rep_weights::get_rep_amounts () const
{
	vxldollar::lock_guard<vxldollar::mutex> guard (mutex);
	return rep_amounts;
}

void vxldollar::rep_weights::copy_from (vxldollar::rep_weights & other_a)
{
	vxldollar::lock_guard<vxldollar::mutex> guard_this (mutex);
	vxldollar::lock_guard<vxldollar::mutex> guard_other (other_a.mutex);
	for (auto const & entry : other_a.rep_amounts)
	{
		auto prev_amount (get (entry.first));
		put (entry.first, prev_amount + entry.second);
	}
}

void vxldollar::rep_weights::put (vxldollar::account const & account_a, vxldollar::uint128_union const & representation_a)
{
	auto it = rep_amounts.find (account_a);
	auto amount = representation_a.number ();
	if (it != rep_amounts.end ())
	{
		it->second = amount;
	}
	else
	{
		rep_amounts.emplace (account_a, amount);
	}
}

vxldollar::uint128_t vxldollar::rep_weights::get (vxldollar::account const & account_a) const
{
	auto it = rep_amounts.find (account_a);
	if (it != rep_amounts.end ())
	{
		return it->second;
	}
	else
	{
		return vxldollar::uint128_t{ 0 };
	}
}

std::unique_ptr<vxldollar::container_info_component> vxldollar::collect_container_info (vxldollar::rep_weights const & rep_weights, std::string const & name)
{
	size_t rep_amounts_count;

	{
		vxldollar::lock_guard<vxldollar::mutex> guard (rep_weights.mutex);
		rep_amounts_count = rep_weights.rep_amounts.size ();
	}
	auto sizeof_element = sizeof (decltype (rep_weights.rep_amounts)::value_type);
	auto composite = std::make_unique<vxldollar::container_info_composite> (name);
	composite->add_component (std::make_unique<vxldollar::container_info_leaf> (container_info{ "rep_amounts", rep_amounts_count, sizeof_element }));
	return composite;
}
