#include <vxldollar/lib/blocks.hpp>
#include <vxldollar/lib/numbers.hpp>
#include <vxldollar/node/ipc/flatbuffers_util.hpp>
#include <vxldollar/secure/common.hpp>

std::unique_ptr<vxldollarapi::BlockStateT> vxldollar::ipc::flatbuffers_builder::from (vxldollar::state_block const & block_a, vxldollar::amount const & amount_a, bool is_state_send_a, bool is_state_epoch_a)
{
	auto block (std::make_unique<vxldollarapi::BlockStateT> ());
	block->account = block_a.account ().to_account ();
	block->hash = block_a.hash ().to_string ();
	block->previous = block_a.previous ().to_string ();
	block->representative = block_a.representative ().to_account ();
	block->balance = block_a.balance ().to_string_dec ();
	block->link = block_a.link ().to_string ();
	block->link_as_account = block_a.link ().to_account ();
	block_a.signature.encode_hex (block->signature);
	block->work = vxldollar::to_string_hex (block_a.work);

	if (is_state_send_a)
	{
		block->subtype = vxldollarapi::BlockSubType::BlockSubType_send;
	}
	else if (block_a.link ().is_zero ())
	{
		block->subtype = vxldollarapi::BlockSubType::BlockSubType_change;
	}
	else if (amount_a == 0 && is_state_epoch_a)
	{
		block->subtype = vxldollarapi::BlockSubType::BlockSubType_epoch;
	}
	else
	{
		block->subtype = vxldollarapi::BlockSubType::BlockSubType_receive;
	}
	return block;
}

std::unique_ptr<vxldollarapi::BlockSendT> vxldollar::ipc::flatbuffers_builder::from (vxldollar::send_block const & block_a)
{
	auto block (std::make_unique<vxldollarapi::BlockSendT> ());
	block->hash = block_a.hash ().to_string ();
	block->balance = block_a.balance ().to_string_dec ();
	block->destination = block_a.hashables.destination.to_account ();
	block->previous = block_a.previous ().to_string ();
	block_a.signature.encode_hex (block->signature);
	block->work = vxldollar::to_string_hex (block_a.work);
	return block;
}

std::unique_ptr<vxldollarapi::BlockReceiveT> vxldollar::ipc::flatbuffers_builder::from (vxldollar::receive_block const & block_a)
{
	auto block (std::make_unique<vxldollarapi::BlockReceiveT> ());
	block->hash = block_a.hash ().to_string ();
	block->source = block_a.source ().to_string ();
	block->previous = block_a.previous ().to_string ();
	block_a.signature.encode_hex (block->signature);
	block->work = vxldollar::to_string_hex (block_a.work);
	return block;
}

std::unique_ptr<vxldollarapi::BlockOpenT> vxldollar::ipc::flatbuffers_builder::from (vxldollar::open_block const & block_a)
{
	auto block (std::make_unique<vxldollarapi::BlockOpenT> ());
	block->hash = block_a.hash ().to_string ();
	block->source = block_a.source ().to_string ();
	block->account = block_a.account ().to_account ();
	block->representative = block_a.representative ().to_account ();
	block_a.signature.encode_hex (block->signature);
	block->work = vxldollar::to_string_hex (block_a.work);
	return block;
}

std::unique_ptr<vxldollarapi::BlockChangeT> vxldollar::ipc::flatbuffers_builder::from (vxldollar::change_block const & block_a)
{
	auto block (std::make_unique<vxldollarapi::BlockChangeT> ());
	block->hash = block_a.hash ().to_string ();
	block->previous = block_a.previous ().to_string ();
	block->representative = block_a.representative ().to_account ();
	block_a.signature.encode_hex (block->signature);
	block->work = vxldollar::to_string_hex (block_a.work);
	return block;
}

vxldollarapi::BlockUnion vxldollar::ipc::flatbuffers_builder::block_to_union (vxldollar::block const & block_a, vxldollar::amount const & amount_a, bool is_state_send_a, bool is_state_epoch_a)
{
	vxldollarapi::BlockUnion u;
	switch (block_a.type ())
	{
		case vxldollar::block_type::state:
		{
			u.Set (*from (dynamic_cast<vxldollar::state_block const &> (block_a), amount_a, is_state_send_a, is_state_epoch_a));
			break;
		}
		case vxldollar::block_type::send:
		{
			u.Set (*from (dynamic_cast<vxldollar::send_block const &> (block_a)));
			break;
		}
		case vxldollar::block_type::receive:
		{
			u.Set (*from (dynamic_cast<vxldollar::receive_block const &> (block_a)));
			break;
		}
		case vxldollar::block_type::open:
		{
			u.Set (*from (dynamic_cast<vxldollar::open_block const &> (block_a)));
			break;
		}
		case vxldollar::block_type::change:
		{
			u.Set (*from (dynamic_cast<vxldollar::change_block const &> (block_a)));
			break;
		}

		default:
			debug_assert (false);
	}
	return u;
}
