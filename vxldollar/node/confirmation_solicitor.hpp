#pragma once

#include <vxldollar/node/network.hpp>
#include <vxldollar/node/repcrawler.hpp>

#include <unordered_map>

namespace vxldollar
{
class election;
class node;
class node_config;
/** This class accepts elections that need further votes before they can be confirmed and bundles them in to single confirm_req packets */
class confirmation_solicitor final
{
public:
	confirmation_solicitor (vxldollar::network &, vxldollar::node_config const &);
	/** Prepare object for batching election confirmation requests*/
	void prepare (std::vector<vxldollar::representative> const &);
	/** Broadcast the winner of an election if the broadcast limit has not been reached. Returns false if the broadcast was performed */
	bool broadcast (vxldollar::election const &);
	/** Add an election that needs to be confirmed. Returns false if successfully added */
	bool add (vxldollar::election const &);
	/** Dispatch bundled requests to each channel*/
	void flush ();
	/** Global maximum amount of block broadcasts */
	std::size_t const max_block_broadcasts;
	/** Maximum amount of requests to be sent per election, bypassed if an existing vote is for a different hash*/
	std::size_t const max_election_requests;
	/** Maximum amount of directed broadcasts to be sent per election */
	std::size_t const max_election_broadcasts;

private:
	vxldollar::network & network;
	vxldollar::node_config const & config;

	unsigned rebroadcasted{ 0 };
	std::vector<vxldollar::representative> representatives_requests;
	std::vector<vxldollar::representative> representatives_broadcasts;
	using vector_root_hashes = std::vector<std::pair<vxldollar::block_hash, vxldollar::root>>;
	std::unordered_map<std::shared_ptr<vxldollar::transport::channel>, vector_root_hashes> requests;
	bool prepared{ false };
};
}
