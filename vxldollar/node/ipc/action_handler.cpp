#include <vxldollar/ipc_flatbuffers_lib/generated/flatbuffers/vxldollarapi_generated.h>
#include <vxldollar/lib/errors.hpp>
#include <vxldollar/lib/numbers.hpp>
#include <vxldollar/node/ipc/action_handler.hpp>
#include <vxldollar/node/ipc/ipc_server.hpp>
#include <vxldollar/node/node.hpp>

#include <iostream>

namespace
{
vxldollar::account parse_account (std::string const & account, bool & out_is_deprecated_format)
{
	vxldollar::account result{};
	if (account.empty ())
	{
		throw vxldollar::error (vxldollar::error_common::bad_account_number);
	}

	if (result.decode_account (account))
	{
		throw vxldollar::error (vxldollar::error_common::bad_account_number);
	}
	else if (account[3] == '-' || account[4] == '-')
	{
		out_is_deprecated_format = true;
	}

	return result;
}
/** Returns the message as a Flatbuffers ObjectAPI type, managed by a unique_ptr */
template <typename T>
auto get_message (vxldollarapi::Envelope const & envelope)
{
	auto raw (envelope.message_as<T> ()->UnPack ());
	return std::unique_ptr<typename T::NativeTableType> (raw);
}
}

/**
 * Mapping from message type to handler function.
 * @note This must be updated whenever a new message type is added to the Flatbuffers IDL.
 */
auto vxldollar::ipc::action_handler::handler_map () -> std::unordered_map<vxldollarapi::Message, std::function<void (vxldollar::ipc::action_handler *, vxldollarapi::Envelope const &)>, vxldollar::ipc::enum_hash>
{
	static std::unordered_map<vxldollarapi::Message, std::function<void (vxldollar::ipc::action_handler *, vxldollarapi::Envelope const &)>, vxldollar::ipc::enum_hash> handlers;
	if (handlers.empty ())
	{
		handlers.emplace (vxldollarapi::Message::Message_IsAlive, &vxldollar::ipc::action_handler::on_is_alive);
		handlers.emplace (vxldollarapi::Message::Message_TopicConfirmation, &vxldollar::ipc::action_handler::on_topic_confirmation);
		handlers.emplace (vxldollarapi::Message::Message_AccountWeight, &vxldollar::ipc::action_handler::on_account_weight);
		handlers.emplace (vxldollarapi::Message::Message_ServiceRegister, &vxldollar::ipc::action_handler::on_service_register);
		handlers.emplace (vxldollarapi::Message::Message_ServiceStop, &vxldollar::ipc::action_handler::on_service_stop);
		handlers.emplace (vxldollarapi::Message::Message_TopicServiceStop, &vxldollar::ipc::action_handler::on_topic_service_stop);
	}
	return handlers;
}

vxldollar::ipc::action_handler::action_handler (vxldollar::node & node_a, vxldollar::ipc::ipc_server & server_a, std::weak_ptr<vxldollar::ipc::subscriber> const & subscriber_a, std::shared_ptr<flatbuffers::FlatBufferBuilder> const & builder_a) :
	flatbuffer_producer (builder_a),
	node (node_a),
	ipc_server (server_a),
	subscriber (subscriber_a)
{
}

void vxldollar::ipc::action_handler::on_topic_confirmation (vxldollarapi::Envelope const & envelope_a)
{
	auto confirmationTopic (get_message<vxldollarapi::TopicConfirmation> (envelope_a));
	ipc_server.get_broker ()->subscribe (subscriber, std::move (confirmationTopic));
	vxldollarapi::EventAckT ack;
	create_response (ack);
}

void vxldollar::ipc::action_handler::on_service_register (vxldollarapi::Envelope const & envelope_a)
{
	require_oneof (envelope_a, { vxldollar::ipc::access_permission::api_service_register, vxldollar::ipc::access_permission::service });
	auto query (get_message<vxldollarapi::ServiceRegister> (envelope_a));
	ipc_server.get_broker ()->service_register (query->service_name, this->subscriber);
	vxldollarapi::SuccessT success;
	create_response (success);
}

void vxldollar::ipc::action_handler::on_service_stop (vxldollarapi::Envelope const & envelope_a)
{
	require_oneof (envelope_a, { vxldollar::ipc::access_permission::api_service_stop, vxldollar::ipc::access_permission::service });
	auto query (get_message<vxldollarapi::ServiceStop> (envelope_a));
	if (query->service_name == "node")
	{
		ipc_server.node.stop ();
	}
	else
	{
		ipc_server.get_broker ()->service_stop (query->service_name);
	}
	vxldollarapi::SuccessT success;
	create_response (success);
}

void vxldollar::ipc::action_handler::on_topic_service_stop (vxldollarapi::Envelope const & envelope_a)
{
	auto topic (get_message<vxldollarapi::TopicServiceStop> (envelope_a));
	ipc_server.get_broker ()->subscribe (subscriber, std::move (topic));
	vxldollarapi::EventAckT ack;
	create_response (ack);
}

void vxldollar::ipc::action_handler::on_account_weight (vxldollarapi::Envelope const & envelope_a)
{
	require_oneof (envelope_a, { vxldollar::ipc::access_permission::api_account_weight, vxldollar::ipc::access_permission::account_query });
	bool is_deprecated_format{ false };
	auto query (get_message<vxldollarapi::AccountWeight> (envelope_a));
	auto balance (node.weight (parse_account (query->account, is_deprecated_format)));

	vxldollarapi::AccountWeightResponseT response;
	response.voting_weight = balance.str ();
	create_response (response);
}

void vxldollar::ipc::action_handler::on_is_alive (vxldollarapi::Envelope const & envelope)
{
	vxldollarapi::IsAliveT alive;
	create_response (alive);
}

bool vxldollar::ipc::action_handler::has_access (vxldollarapi::Envelope const & envelope_a, vxldollar::ipc::access_permission permission_a) const noexcept
{
	// If credentials are missing in the envelope, the default user is used
	std::string credentials;
	if (envelope_a.credentials () != nullptr)
	{
		credentials = envelope_a.credentials ()->str ();
	}

	return ipc_server.get_access ().has_access (credentials, permission_a);
}

bool vxldollar::ipc::action_handler::has_access_to_all (vxldollarapi::Envelope const & envelope_a, std::initializer_list<vxldollar::ipc::access_permission> permissions_a) const noexcept
{
	// If credentials are missing in the envelope, the default user is used
	std::string credentials;
	if (envelope_a.credentials () != nullptr)
	{
		credentials = envelope_a.credentials ()->str ();
	}

	return ipc_server.get_access ().has_access_to_all (credentials, permissions_a);
}

bool vxldollar::ipc::action_handler::has_access_to_oneof (vxldollarapi::Envelope const & envelope_a, std::initializer_list<vxldollar::ipc::access_permission> permissions_a) const noexcept
{
	// If credentials are missing in the envelope, the default user is used
	std::string credentials;
	if (envelope_a.credentials () != nullptr)
	{
		credentials = envelope_a.credentials ()->str ();
	}

	return ipc_server.get_access ().has_access_to_oneof (credentials, permissions_a);
}

void vxldollar::ipc::action_handler::require (vxldollarapi::Envelope const & envelope_a, vxldollar::ipc::access_permission permission_a) const
{
	if (!has_access (envelope_a, permission_a))
	{
		throw vxldollar::error (vxldollar::error_common::access_denied);
	}
}

void vxldollar::ipc::action_handler::require_all (vxldollarapi::Envelope const & envelope_a, std::initializer_list<vxldollar::ipc::access_permission> permissions_a) const
{
	if (!has_access_to_all (envelope_a, permissions_a))
	{
		throw vxldollar::error (vxldollar::error_common::access_denied);
	}
}

void vxldollar::ipc::action_handler::require_oneof (vxldollarapi::Envelope const & envelope_a, std::initializer_list<vxldollar::ipc::access_permission> permissions_a) const
{
	if (!has_access_to_oneof (envelope_a, permissions_a))
	{
		throw vxldollar::error (vxldollar::error_common::access_denied);
	}
}
