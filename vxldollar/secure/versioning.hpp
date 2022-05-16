#pragma once

#include <vxldollar/lib/blocks.hpp>
#include <vxldollar/secure/common.hpp>

struct MDB_val;

namespace vxldollar
{
class pending_info_v14 final
{
public:
	pending_info_v14 () = default;
	pending_info_v14 (vxldollar::account const &, vxldollar::amount const &, vxldollar::epoch);
	size_t db_size () const;
	bool deserialize (vxldollar::stream &);
	bool operator== (vxldollar::pending_info_v14 const &) const;
	vxldollar::account source{};
	vxldollar::amount amount{ 0 };
	vxldollar::epoch epoch{ vxldollar::epoch::epoch_0 };
};
class account_info_v14 final
{
public:
	account_info_v14 () = default;
	account_info_v14 (vxldollar::block_hash const &, vxldollar::block_hash const &, vxldollar::block_hash const &, vxldollar::amount const &, uint64_t, uint64_t, uint64_t, vxldollar::epoch);
	size_t db_size () const;
	vxldollar::block_hash head{ 0 };
	vxldollar::block_hash rep_block{ 0 };
	vxldollar::block_hash open_block{ 0 };
	vxldollar::amount balance{ 0 };
	uint64_t modified{ 0 };
	uint64_t block_count{ 0 };
	uint64_t confirmation_height{ 0 };
	vxldollar::epoch epoch{ vxldollar::epoch::epoch_0 };
};
class block_sideband_v14 final
{
public:
	block_sideband_v14 () = default;
	block_sideband_v14 (vxldollar::block_type, vxldollar::account const &, vxldollar::block_hash const &, vxldollar::amount const &, uint64_t, uint64_t);
	void serialize (vxldollar::stream &) const;
	bool deserialize (vxldollar::stream &);
	static size_t size (vxldollar::block_type);
	vxldollar::block_type type{ vxldollar::block_type::invalid };
	vxldollar::block_hash successor{ 0 };
	vxldollar::account account{};
	vxldollar::amount balance{ 0 };
	uint64_t height{ 0 };
	uint64_t timestamp{ 0 };
};
class state_block_w_sideband_v14
{
public:
	std::shared_ptr<vxldollar::state_block> state_block;
	vxldollar::block_sideband_v14 sideband;
};
class block_sideband_v18 final
{
public:
	block_sideband_v18 () = default;
	block_sideband_v18 (vxldollar::account const &, vxldollar::block_hash const &, vxldollar::amount const &, uint64_t, uint64_t, vxldollar::block_details const &);
	block_sideband_v18 (vxldollar::account const &, vxldollar::block_hash const &, vxldollar::amount const &, uint64_t, uint64_t, vxldollar::epoch, bool is_send, bool is_receive, bool is_epoch);
	void serialize (vxldollar::stream &, vxldollar::block_type) const;
	bool deserialize (vxldollar::stream &, vxldollar::block_type);
	static size_t size (vxldollar::block_type);
	vxldollar::block_hash successor{ 0 };
	vxldollar::account account{};
	vxldollar::amount balance{ 0 };
	uint64_t height{ 0 };
	uint64_t timestamp{ 0 };
	vxldollar::block_details details;
};
}
