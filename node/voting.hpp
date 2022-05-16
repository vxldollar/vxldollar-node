#pragma once

#include <vxldollar/lib/locks.hpp>
#include <vxldollar/lib/numbers.hpp>
#include <vxldollar/lib/utility.hpp>
#include <vxldollar/node/wallet.hpp>
#include <vxldollar/secure/common.hpp>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>

#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

namespace mi = boost::multi_index;

namespace vxldollar
{
class ledger;
class network;
class node_config;
class stat;
class vote_processor;
class wallets;
namespace transport
{
	class channel;
}

class vote_spacing final
{
	class entry
	{
	public:
		vxldollar::root root;
		std::chrono::steady_clock::time_point time;
		vxldollar::block_hash hash;
	};

	boost::multi_index_container<entry,
	mi::indexed_by<
	mi::hashed_non_unique<mi::tag<class tag_root>,
	mi::member<entry, vxldollar::root, &entry::root>>,
	mi::ordered_non_unique<mi::tag<class tag_time>,
	mi::member<entry, std::chrono::steady_clock::time_point, &entry::time>>>>
	recent;
	std::chrono::milliseconds const delay;
	void trim ();

public:
	vote_spacing (std::chrono::milliseconds const & delay) :
		delay{ delay }
	{
	}
	bool votable (vxldollar::root const & root_a, vxldollar::block_hash const & hash_a) const;
	void flag (vxldollar::root const & root_a, vxldollar::block_hash const & hash_a);
	std::size_t size () const;
};

class local_vote_history final
{
	class local_vote final
	{
	public:
		local_vote (vxldollar::root const & root_a, vxldollar::block_hash const & hash_a, std::shared_ptr<vxldollar::vote> const & vote_a) :
			root (root_a),
			hash (hash_a),
			vote (vote_a)
		{
		}
		vxldollar::root root;
		vxldollar::block_hash hash;
		std::shared_ptr<vxldollar::vote> vote;
	};

public:
	local_vote_history (vxldollar::voting_constants const & constants) :
		constants{ constants }
	{
	}
	void add (vxldollar::root const & root_a, vxldollar::block_hash const & hash_a, std::shared_ptr<vxldollar::vote> const & vote_a);
	void erase (vxldollar::root const & root_a);

	std::vector<std::shared_ptr<vxldollar::vote>> votes (vxldollar::root const & root_a, vxldollar::block_hash const & hash_a, bool const is_final_a = false) const;
	bool exists (vxldollar::root const &) const;
	std::size_t size () const;

private:
	// clang-format off
	boost::multi_index_container<local_vote,
	mi::indexed_by<
		mi::hashed_non_unique<mi::tag<class tag_root>,
			mi::member<local_vote, vxldollar::root, &local_vote::root>>,
		mi::sequenced<mi::tag<class tag_sequence>>>>
	history;
	// clang-format on

	vxldollar::voting_constants const & constants;
	void clean ();
	std::vector<std::shared_ptr<vxldollar::vote>> votes (vxldollar::root const & root_a) const;
	// Only used in Debug
	bool consistency_check (vxldollar::root const &) const;
	mutable vxldollar::mutex mutex;

	friend std::unique_ptr<container_info_component> collect_container_info (local_vote_history & history, std::string const & name);
	friend class local_vote_history_basic_Test;
};

std::unique_ptr<container_info_component> collect_container_info (local_vote_history & history, std::string const & name);

class vote_generator final
{
private:
	using candidate_t = std::pair<vxldollar::root, vxldollar::block_hash>;
	using request_t = std::pair<std::vector<candidate_t>, std::shared_ptr<vxldollar::transport::channel>>;

public:
	vote_generator (vxldollar::node_config const & config_a, vxldollar::ledger & ledger_a, vxldollar::wallets & wallets_a, vxldollar::vote_processor & vote_processor_a, vxldollar::local_vote_history & history_a, vxldollar::network & network_a, vxldollar::stat & stats_a, bool is_final_a);
	/** Queue items for vote generation, or broadcast votes already in cache */
	void add (vxldollar::root const &, vxldollar::block_hash const &);
	/** Queue blocks for vote generation, returning the number of successful candidates.*/
	std::size_t generate (std::vector<std::shared_ptr<vxldollar::block>> const & blocks_a, std::shared_ptr<vxldollar::transport::channel> const & channel_a);
	void set_reply_action (std::function<void (std::shared_ptr<vxldollar::vote> const &, std::shared_ptr<vxldollar::transport::channel> const &)>);
	void stop ();

private:
	void run ();
	void broadcast (vxldollar::unique_lock<vxldollar::mutex> &);
	void reply (vxldollar::unique_lock<vxldollar::mutex> &, request_t &&);
	void vote (std::vector<vxldollar::block_hash> const &, std::vector<vxldollar::root> const &, std::function<void (std::shared_ptr<vxldollar::vote> const &)> const &);
	void broadcast_action (std::shared_ptr<vxldollar::vote> const &) const;
	std::function<void (std::shared_ptr<vxldollar::vote> const &, std::shared_ptr<vxldollar::transport::channel> &)> reply_action; // must be set only during initialization by using set_reply_action
	vxldollar::node_config const & config;
	vxldollar::ledger & ledger;
	vxldollar::wallets & wallets;
	vxldollar::vote_processor & vote_processor;
	vxldollar::local_vote_history & history;
	vxldollar::vote_spacing spacing;
	vxldollar::network & network;
	vxldollar::stat & stats;
	mutable vxldollar::mutex mutex;
	vxldollar::condition_variable condition;
	static std::size_t constexpr max_requests{ 2048 };
	std::deque<request_t> requests;
	std::deque<candidate_t> candidates;
	std::atomic<bool> stopped{ false };
	bool started{ false };
	std::thread thread;
	bool is_final;

	friend std::unique_ptr<container_info_component> collect_container_info (vote_generator & vote_generator, std::string const & name);
};

std::unique_ptr<container_info_component> collect_container_info (vote_generator & generator, std::string const & name);

class vote_generator_session final
{
public:
	vote_generator_session (vote_generator & vote_generator_a);
	void add (vxldollar::root const &, vxldollar::block_hash const &);
	void flush ();

private:
	vxldollar::vote_generator & generator;
	std::vector<std::pair<vxldollar::root, vxldollar::block_hash>> items;
};
}
