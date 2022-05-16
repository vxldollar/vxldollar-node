#pragma once

#include <vxldollar/lib/numbers.hpp>
#include <vxldollar/lib/utility.hpp>
#include <vxldollar/secure/common.hpp>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>

#include <memory>
#include <unordered_set>
#include <vector>

namespace vxldollar
{
class ledger;
class node_config;
class transaction;

/** Track online representatives and trend online weight */
class online_reps final
{
public:
	online_reps (vxldollar::ledger & ledger_a, vxldollar::node_config const & config_a);
	/** Add voting account \p rep_account to the set of online representatives */
	void observe (vxldollar::account const & rep_account);
	/** Called periodically to sample online weight */
	void sample ();
	/** Returns the trended online stake */
	vxldollar::uint128_t trended () const;
	/** Returns the current online stake */
	vxldollar::uint128_t online () const;
	/** Returns the quorum required for confirmation*/
	vxldollar::uint128_t delta () const;
	/** List of online representatives, both the currently sampling ones and the ones observed in the previous sampling period */
	std::vector<vxldollar::account> list ();
	void clear ();
	static unsigned constexpr online_weight_quorum = 34; //

private:
	class rep_info
	{
	public:
		std::chrono::steady_clock::time_point time;
		vxldollar::account account;
	};
	class tag_time
	{
	};
	class tag_account
	{
	};
	vxldollar::uint128_t calculate_trend (vxldollar::transaction &) const;
	vxldollar::uint128_t calculate_online () const;
	mutable vxldollar::mutex mutex;
	vxldollar::ledger & ledger;
	vxldollar::node_config const & config;
	boost::multi_index_container<rep_info,
	boost::multi_index::indexed_by<
	boost::multi_index::ordered_non_unique<boost::multi_index::tag<tag_time>,
	boost::multi_index::member<rep_info, std::chrono::steady_clock::time_point, &rep_info::time>>,
	boost::multi_index::hashed_unique<boost::multi_index::tag<tag_account>,
	boost::multi_index::member<rep_info, vxldollar::account, &rep_info::account>>>>
	reps;
	vxldollar::uint128_t trended_m;
	vxldollar::uint128_t online_m;
	vxldollar::uint128_t minimum;

	friend class election_quorum_minimum_update_weight_before_quorum_checks_Test;
	friend std::unique_ptr<container_info_component> collect_container_info (online_reps & online_reps, std::string const & name);
};

std::unique_ptr<container_info_component> collect_container_info (online_reps & online_reps, std::string const & name);
}
