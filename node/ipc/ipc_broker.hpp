#pragma once

#include <vxldollar/ipc_flatbuffers_lib/generated/flatbuffers/vxldollarapi_generated.h>
#include <vxldollar/lib/ipc.hpp>
#include <vxldollar/lib/locks.hpp>
#include <vxldollar/node/ipc/ipc_broker.hpp>
#include <vxldollar/node/node_rpc_config.hpp>

#include <atomic>
#include <mutex>

namespace flatbuffers
{
class Parser;
}
namespace vxldollar
{
class node;
class error;
namespace ipc
{
	class ipc_config;
	/**
	 * A subscriber represents a live session, and is weakly referenced vxldollar::ipc::subscription whenever a subscription is made.
	 * This construction helps making the session implementation opaque to clients.
	 */
	class subscriber
	{
	public:
		virtual ~subscriber () = default;

		/**
		 * Send message payload to the client. The implementation will prepend the bigendian length.
		 * @param data_a The caller must ensure the lifetime is extended until the completion handler is called, such as through a lambda capture.
		 * @param length_a Length of payload message in bytes
		 * @param broadcast_completion_handler_a Called once sending is completed
		 */
		virtual void async_send_message (uint8_t const * data_a, std::size_t length_a, std::function<void (vxldollar::error const &)> broadcast_completion_handler_a) = 0;
		/** Returns the unique id of the associated session */
		virtual uint64_t get_id () const = 0;
		/** Returns the service name associated with the session */
		virtual std::string get_service_name () const = 0;
		/** Sets the service name associated with the session */
		virtual void set_service_name (std::string const & service_name_a) = 0;
		/** Returns the session's active payload encoding */
		virtual vxldollar::ipc::payload_encoding get_active_encoding () const = 0;

		/** Get flatbuffer parser instance for this subscriber; create it if necessary */
		std::shared_ptr<flatbuffers::Parser> get_parser (vxldollar::ipc::ipc_config const & ipc_config_a);

	private:
		std::shared_ptr<flatbuffers::Parser> parser;
	};

	/**
	 * Subscriptions are added to the broker whenever a topic message is sent from a client.
	 * The subscription is removed when the client unsubscribes, or lazily removed after the
	 * session is closed.
	 */
	template <typename TopicType>
	class subscription final
	{
	public:
		subscription (std::weak_ptr<vxldollar::ipc::subscriber> const & subscriber_a, std::shared_ptr<TopicType> const & topic_a) :
			subscriber (subscriber_a), topic (topic_a)
		{
		}

		std::weak_ptr<vxldollar::ipc::subscriber> subscriber;
		std::shared_ptr<TopicType> topic;
	};

	/**
	 * The broker manages subscribers and performs message broadcasting
	 * @note Add subscribe overloads for new topics
	 */
	class broker final : public std::enable_shared_from_this<broker>
	{
	public:
		broker (vxldollar::node & node_a);
		/** Starts the broker by setting up observers */
		void start ();
		/** Subscribe to block confirmations */
		void subscribe (std::weak_ptr<vxldollar::ipc::subscriber> const & subscriber_a, std::shared_ptr<vxldollarapi::TopicConfirmationT> const & confirmation_a);
		/** Subscribe to EventServiceStop notifications for \p subscriber_a. The subscriber must first have called ServiceRegister. */
		void subscribe (std::weak_ptr<vxldollar::ipc::subscriber> const & subscriber_a, std::shared_ptr<vxldollarapi::TopicServiceStopT> const & service_stop_a);

		/** Returns the number of confirmation subscribers */
		std::size_t confirmation_subscriber_count () const;
		/** Associate the service name with the subscriber */
		void service_register (std::string const & service_name_a, std::weak_ptr<vxldollar::ipc::subscriber> const & subscriber_a);
		/** Sends a notification to the session associated with the given service (if the session has subscribed to TopicServiceStop) */
		void service_stop (std::string const & service_name_a);

	private:
		/** Broadcast block confirmations */
		void broadcast (std::shared_ptr<vxldollarapi::EventConfirmationT> const & confirmation_a);

		vxldollar::node & node;
		mutable vxldollar::locked<std::vector<subscription<vxldollarapi::TopicConfirmationT>>> confirmation_subscribers;
		mutable vxldollar::locked<std::vector<subscription<vxldollarapi::TopicServiceStopT>>> service_stop_subscribers;
	};
}
}
