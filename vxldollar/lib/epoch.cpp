#include <vxldollar/lib/epoch.hpp>
#include <vxldollar/lib/utility.hpp>

vxldollar::link const & vxldollar::epochs::link (vxldollar::epoch epoch_a) const
{
	return epochs_m.at (epoch_a).link;
}

bool vxldollar::epochs::is_epoch_link (vxldollar::link const & link_a) const
{
	return std::any_of (epochs_m.begin (), epochs_m.end (), [&link_a] (auto const & item_a) { return item_a.second.link == link_a; });
}

vxldollar::public_key const & vxldollar::epochs::signer (vxldollar::epoch epoch_a) const
{
	return epochs_m.at (epoch_a).signer;
}

vxldollar::epoch vxldollar::epochs::epoch (vxldollar::link const & link_a) const
{
	auto existing (std::find_if (epochs_m.begin (), epochs_m.end (), [&link_a] (auto const & item_a) { return item_a.second.link == link_a; }));
	debug_assert (existing != epochs_m.end ());
	return existing->first;
}

void vxldollar::epochs::add (vxldollar::epoch epoch_a, vxldollar::public_key const & signer_a, vxldollar::link const & link_a)
{
	debug_assert (epochs_m.find (epoch_a) == epochs_m.end ());
	epochs_m[epoch_a] = { signer_a, link_a };
}

bool vxldollar::epochs::is_sequential (vxldollar::epoch epoch_a, vxldollar::epoch new_epoch_a)
{
	auto head_epoch = std::underlying_type_t<vxldollar::epoch> (epoch_a);
	bool is_valid_epoch (head_epoch >= std::underlying_type_t<vxldollar::epoch> (vxldollar::epoch::epoch_0));
	return is_valid_epoch && (std::underlying_type_t<vxldollar::epoch> (new_epoch_a) == (head_epoch + 1));
}

std::underlying_type_t<vxldollar::epoch> vxldollar::normalized_epoch (vxldollar::epoch epoch_a)
{
	// Currently assumes that the epoch versions in the enum are sequential.
	auto start = std::underlying_type_t<vxldollar::epoch> (vxldollar::epoch::epoch_0);
	auto end = std::underlying_type_t<vxldollar::epoch> (epoch_a);
	debug_assert (end >= start);
	return end - start;
}
