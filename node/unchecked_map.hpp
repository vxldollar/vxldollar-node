#pragma once

#include <vxldollar/lib/locks.hpp>
#include <vxldollar/lib/numbers.hpp>
#include <vxldollar/secure/store.hpp>

#include <atomic>
#include <thread>
#include <unordered_map>

namespace vxldollar
{
class store;
class transaction;
class unchecked_info;
class unchecked_key;
class write_transaction;
class unchecked_map
{
public:
	using iterator = vxldollar::unchecked_store::iterator;

public:
	unchecked_map (vxldollar::store & store, bool const & do_delete);
	~unchecked_map ();
	void put (vxldollar::hash_or_account const & dependency, vxldollar::unchecked_info const & info);
	std::pair<iterator, iterator> equal_range (vxldollar::transaction const & transaction, vxldollar::block_hash const & dependency);
	std::pair<iterator, iterator> full_range (vxldollar::transaction const & transaction);
	std::vector<vxldollar::unchecked_info> get (vxldollar::transaction const &, vxldollar::block_hash const &);
	bool exists (vxldollar::transaction const & transaction, vxldollar::unchecked_key const & key) const;
	void del (vxldollar::write_transaction const & transaction, vxldollar::unchecked_key const & key);
	void clear (vxldollar::write_transaction const & transaction);
	size_t count (vxldollar::transaction const & transaction) const;
	void stop ();
	void flush ();

public: // Trigger requested dependencies
	void trigger (vxldollar::hash_or_account const & dependency);
	std::function<void (vxldollar::unchecked_info const &)> satisfied{ [] (vxldollar::unchecked_info const &) {} };

private:
	using insert = std::pair<vxldollar::hash_or_account, vxldollar::unchecked_info>;
	using query = vxldollar::hash_or_account;
	class item_visitor : boost::static_visitor<>
	{
	public:
		item_visitor (unchecked_map & unchecked, vxldollar::write_transaction const & transaction);
		void operator() (insert const & item);
		void operator() (query const & item);
		unchecked_map & unchecked;
		vxldollar::write_transaction const & transaction;
	};
	void run ();
	vxldollar::store & store;
	bool const & disable_delete;
	std::deque<boost::variant<insert, query>> buffer;
	std::deque<boost::variant<insert, query>> back_buffer;
	bool writing_back_buffer{ false };
	bool stopped{ false };
	vxldollar::condition_variable condition;
	vxldollar::mutex mutex;
	std::thread thread;
	void write_buffer (decltype (buffer) const & back_buffer);
};
}
