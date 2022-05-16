#pragma once

#include <vxldollar/node/common.hpp>
#include <vxldollar/node/socket.hpp>

#include <unordered_set>

namespace vxldollar
{
class bootstrap_attempt;
class pull_info
{
public:
	using count_t = vxldollar::bulk_pull::count_t;
	pull_info () = default;
	pull_info (vxldollar::hash_or_account const &, vxldollar::block_hash const &, vxldollar::block_hash const &, uint64_t, count_t = 0, unsigned = 16);
	vxldollar::hash_or_account account_or_head{ 0 };
	vxldollar::block_hash head{ 0 };
	vxldollar::block_hash head_original{ 0 };
	vxldollar::block_hash end{ 0 };
	count_t count{ 0 };
	unsigned attempts{ 0 };
	uint64_t processed{ 0 };
	unsigned retry_limit{ 0 };
	uint64_t bootstrap_id{ 0 };
};
class bootstrap_client;

/**
 * Client side of a bulk_pull request. Created when the bootstrap_attempt wants to make a bulk_pull request to the remote side.
 */
class bulk_pull_client final : public std::enable_shared_from_this<vxldollar::bulk_pull_client>
{
public:
	bulk_pull_client (std::shared_ptr<vxldollar::bootstrap_client> const &, std::shared_ptr<vxldollar::bootstrap_attempt> const &, vxldollar::pull_info const &);
	~bulk_pull_client ();
	void request ();
	void receive_block ();
	void throttled_receive_block ();
	void received_type ();
	void received_block (boost::system::error_code const &, std::size_t, vxldollar::block_type);
	vxldollar::block_hash first ();
	std::shared_ptr<vxldollar::bootstrap_client> connection;
	std::shared_ptr<vxldollar::bootstrap_attempt> attempt;
	vxldollar::block_hash expected;
	vxldollar::account known_account;
	vxldollar::pull_info pull;
	uint64_t pull_blocks;
	uint64_t unexpected_count;
	bool network_error{ false };
};
class bulk_pull_account_client final : public std::enable_shared_from_this<vxldollar::bulk_pull_account_client>
{
public:
	bulk_pull_account_client (std::shared_ptr<vxldollar::bootstrap_client> const &, std::shared_ptr<vxldollar::bootstrap_attempt> const &, vxldollar::account const &);
	~bulk_pull_account_client ();
	void request ();
	void receive_pending ();
	std::shared_ptr<vxldollar::bootstrap_client> connection;
	std::shared_ptr<vxldollar::bootstrap_attempt> attempt;
	vxldollar::account account;
	uint64_t pull_blocks;
};
class bootstrap_server;
class bulk_pull;

/**
 * Server side of a bulk_pull request. Created when bootstrap_server receives a bulk_pull message and is exited after the contents
 * have been sent. If the 'start' in the bulk_pull message is an account, send blocks for that account down to 'end'. If the 'start'
 * is a block hash, send blocks for that chain down to 'end'. If end doesn't exist, send all accounts in the chain.
 */
class bulk_pull_server final : public std::enable_shared_from_this<vxldollar::bulk_pull_server>
{
public:
	bulk_pull_server (std::shared_ptr<vxldollar::bootstrap_server> const &, std::unique_ptr<vxldollar::bulk_pull>);
	void set_current_end ();
	std::shared_ptr<vxldollar::block> get_next ();
	void send_next ();
	void sent_action (boost::system::error_code const &, std::size_t);
	void send_finished ();
	void no_block_sent (boost::system::error_code const &, std::size_t);
	std::shared_ptr<vxldollar::bootstrap_server> connection;
	std::unique_ptr<vxldollar::bulk_pull> request;
	vxldollar::block_hash current;
	bool include_start;
	vxldollar::bulk_pull::count_t max_count;
	vxldollar::bulk_pull::count_t sent_count;
};
class bulk_pull_account;
class bulk_pull_account_server final : public std::enable_shared_from_this<vxldollar::bulk_pull_account_server>
{
public:
	bulk_pull_account_server (std::shared_ptr<vxldollar::bootstrap_server> const &, std::unique_ptr<vxldollar::bulk_pull_account>);
	void set_params ();
	std::pair<std::unique_ptr<vxldollar::pending_key>, std::unique_ptr<vxldollar::pending_info>> get_next ();
	void send_frontier ();
	void send_next_block ();
	void sent_action (boost::system::error_code const &, std::size_t);
	void send_finished ();
	void complete (boost::system::error_code const &, std::size_t);
	std::shared_ptr<vxldollar::bootstrap_server> connection;
	std::unique_ptr<vxldollar::bulk_pull_account> request;
	std::unordered_set<vxldollar::uint256_union> deduplication;
	vxldollar::pending_key current_key;
	bool pending_address_only;
	bool pending_include_address;
	bool invalid_request;
};
}
