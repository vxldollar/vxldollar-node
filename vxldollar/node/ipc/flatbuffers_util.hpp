#pragma once

#include <vxldollar/ipc_flatbuffers_lib/generated/flatbuffers/vxldollarapi_generated.h>

#include <memory>

namespace vxldollar
{
class amount;
class block;
class send_block;
class receive_block;
class change_block;
class open_block;
class state_block;
namespace ipc
{
	/**
	 * Utilities to convert between blocks and Flatbuffers equivalents
	 */
	class flatbuffers_builder
	{
	public:
		static vxldollarapi::BlockUnion block_to_union (vxldollar::block const & block_a, vxldollar::amount const & amount_a, bool is_state_send_a = false, bool is_state_epoch_a = false);
		static std::unique_ptr<vxldollarapi::BlockStateT> from (vxldollar::state_block const & block_a, vxldollar::amount const & amount_a, bool is_state_send_a, bool is_state_epoch_a);
		static std::unique_ptr<vxldollarapi::BlockSendT> from (vxldollar::send_block const & block_a);
		static std::unique_ptr<vxldollarapi::BlockReceiveT> from (vxldollar::receive_block const & block_a);
		static std::unique_ptr<vxldollarapi::BlockOpenT> from (vxldollar::open_block const & block_a);
		static std::unique_ptr<vxldollarapi::BlockChangeT> from (vxldollar::change_block const & block_a);
	};
}
}
