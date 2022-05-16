#pragma once

#include <vxldollar/crypto/blake2/blake2.h>
#include <vxldollar/lib/epoch.hpp>
#include <vxldollar/lib/errors.hpp>
#include <vxldollar/lib/numbers.hpp>
#include <vxldollar/lib/optional_ptr.hpp>
#include <vxldollar/lib/stream.hpp>
#include <vxldollar/lib/utility.hpp>
#include <vxldollar/lib/work.hpp>

#include <boost/property_tree/ptree_fwd.hpp>

#include <unordered_map>

namespace vxldollar
{
class block_visitor;
class mutable_block_visitor;
enum class block_type : uint8_t
{
	invalid = 0,
	not_a_block = 1,
	send = 2,
	receive = 3,
	open = 4,
	change = 5,
	state = 6
};
class block_details
{
	static_assert (std::is_same<std::underlying_type<vxldollar::epoch>::type, uint8_t> (), "Epoch enum is not the proper type");
	static_assert (static_cast<uint8_t> (vxldollar::epoch::max) < (1 << 5), "Epoch max is too large for the sideband");

public:
	block_details () = default;
	block_details (vxldollar::epoch const epoch_a, bool const is_send_a, bool const is_receive_a, bool const is_epoch_a);
	static constexpr size_t size ()
	{
		return 1;
	}
	bool operator== (block_details const & other_a) const;
	void serialize (vxldollar::stream &) const;
	bool deserialize (vxldollar::stream &);
	vxldollar::epoch epoch{ vxldollar::epoch::epoch_0 };
	bool is_send{ false };
	bool is_receive{ false };
	bool is_epoch{ false };

private:
	uint8_t packed () const;
	void unpack (uint8_t);
};

std::string state_subtype (vxldollar::block_details const);

class block_sideband final
{
public:
	block_sideband () = default;
	block_sideband (vxldollar::account const &, vxldollar::block_hash const &, vxldollar::amount const &, uint64_t const, uint64_t const, vxldollar::block_details const &, vxldollar::epoch const source_epoch_a);
	block_sideband (vxldollar::account const &, vxldollar::block_hash const &, vxldollar::amount const &, uint64_t const, uint64_t const, vxldollar::epoch const epoch_a, bool const is_send, bool const is_receive, bool const is_epoch, vxldollar::epoch const source_epoch_a);
	void serialize (vxldollar::stream &, vxldollar::block_type) const;
	bool deserialize (vxldollar::stream &, vxldollar::block_type);
	static size_t size (vxldollar::block_type);
	vxldollar::block_hash successor{ 0 };
	vxldollar::account account{};
	vxldollar::amount balance{ 0 };
	uint64_t height{ 0 };
	uint64_t timestamp{ 0 };
	vxldollar::block_details details;
	vxldollar::epoch source_epoch{ vxldollar::epoch::epoch_0 };
};
class block
{
public:
	// Return a digest of the hashables in this block.
	vxldollar::block_hash const & hash () const;
	// Return a digest of hashables and non-hashables in this block.
	vxldollar::block_hash full_hash () const;
	vxldollar::block_sideband const & sideband () const;
	void sideband_set (vxldollar::block_sideband const &);
	bool has_sideband () const;
	std::string to_json () const;
	virtual void hash (blake2b_state &) const = 0;
	virtual uint64_t block_work () const = 0;
	virtual void block_work_set (uint64_t) = 0;
	virtual vxldollar::account const & account () const;
	// Previous block in account's chain, zero for open block
	virtual vxldollar::block_hash const & previous () const = 0;
	// Source block for open/receive blocks, zero otherwise.
	virtual vxldollar::block_hash const & source () const;
	// Destination account for send blocks, zero otherwise.
	virtual vxldollar::account const & destination () const;
	// Previous block or account number for open blocks
	virtual vxldollar::root const & root () const = 0;
	// Qualified root value based on previous() and root()
	virtual vxldollar::qualified_root qualified_root () const;
	// Link field for state blocks, zero otherwise.
	virtual vxldollar::link const & link () const;
	virtual vxldollar::account const & representative () const;
	virtual vxldollar::amount const & balance () const;
	virtual void serialize (vxldollar::stream &) const = 0;
	virtual void serialize_json (std::string &, bool = false) const = 0;
	virtual void serialize_json (boost::property_tree::ptree &) const = 0;
	virtual void visit (vxldollar::block_visitor &) const = 0;
	virtual void visit (vxldollar::mutable_block_visitor &) = 0;
	virtual bool operator== (vxldollar::block const &) const = 0;
	virtual vxldollar::block_type type () const = 0;
	virtual vxldollar::signature const & block_signature () const = 0;
	virtual void signature_set (vxldollar::signature const &) = 0;
	virtual ~block () = default;
	virtual bool valid_predecessor (vxldollar::block const &) const = 0;
	static size_t size (vxldollar::block_type);
	virtual vxldollar::work_version work_version () const;
	// If there are any changes to the hashables, call this to update the cached hash
	void refresh ();

protected:
	mutable vxldollar::block_hash cached_hash{ 0 };
	/**
	 * Contextual details about a block, some fields may or may not be set depending on block type.
	 * This field is set via sideband_set in ledger processing or deserializing blocks from the database.
	 * Otherwise it may be null (for example, an old block or fork).
	 */
	vxldollar::optional_ptr<vxldollar::block_sideband> sideband_m;

private:
	vxldollar::block_hash generate_hash () const;
};
class send_hashables
{
public:
	send_hashables () = default;
	send_hashables (vxldollar::block_hash const &, vxldollar::account const &, vxldollar::amount const &);
	send_hashables (bool &, vxldollar::stream &);
	send_hashables (bool &, boost::property_tree::ptree const &);
	void hash (blake2b_state &) const;
	vxldollar::block_hash previous;
	vxldollar::account destination;
	vxldollar::amount balance;
	static std::size_t constexpr size = sizeof (previous) + sizeof (destination) + sizeof (balance);
};
class send_block : public vxldollar::block
{
public:
	send_block () = default;
	send_block (vxldollar::block_hash const &, vxldollar::account const &, vxldollar::amount const &, vxldollar::raw_key const &, vxldollar::public_key const &, uint64_t);
	send_block (bool &, vxldollar::stream &);
	send_block (bool &, boost::property_tree::ptree const &);
	virtual ~send_block () = default;
	using vxldollar::block::hash;
	void hash (blake2b_state &) const override;
	uint64_t block_work () const override;
	void block_work_set (uint64_t) override;
	vxldollar::block_hash const & previous () const override;
	vxldollar::account const & destination () const override;
	vxldollar::root const & root () const override;
	vxldollar::amount const & balance () const override;
	void serialize (vxldollar::stream &) const override;
	bool deserialize (vxldollar::stream &);
	void serialize_json (std::string &, bool = false) const override;
	void serialize_json (boost::property_tree::ptree &) const override;
	bool deserialize_json (boost::property_tree::ptree const &);
	void visit (vxldollar::block_visitor &) const override;
	void visit (vxldollar::mutable_block_visitor &) override;
	vxldollar::block_type type () const override;
	vxldollar::signature const & block_signature () const override;
	void signature_set (vxldollar::signature const &) override;
	bool operator== (vxldollar::block const &) const override;
	bool operator== (vxldollar::send_block const &) const;
	bool valid_predecessor (vxldollar::block const &) const override;
	send_hashables hashables;
	vxldollar::signature signature;
	uint64_t work;
	static std::size_t constexpr size = vxldollar::send_hashables::size + sizeof (signature) + sizeof (work);
};
class receive_hashables
{
public:
	receive_hashables () = default;
	receive_hashables (vxldollar::block_hash const &, vxldollar::block_hash const &);
	receive_hashables (bool &, vxldollar::stream &);
	receive_hashables (bool &, boost::property_tree::ptree const &);
	void hash (blake2b_state &) const;
	vxldollar::block_hash previous;
	vxldollar::block_hash source;
	static std::size_t constexpr size = sizeof (previous) + sizeof (source);
};
class receive_block : public vxldollar::block
{
public:
	receive_block () = default;
	receive_block (vxldollar::block_hash const &, vxldollar::block_hash const &, vxldollar::raw_key const &, vxldollar::public_key const &, uint64_t);
	receive_block (bool &, vxldollar::stream &);
	receive_block (bool &, boost::property_tree::ptree const &);
	virtual ~receive_block () = default;
	using vxldollar::block::hash;
	void hash (blake2b_state &) const override;
	uint64_t block_work () const override;
	void block_work_set (uint64_t) override;
	vxldollar::block_hash const & previous () const override;
	vxldollar::block_hash const & source () const override;
	vxldollar::root const & root () const override;
	void serialize (vxldollar::stream &) const override;
	bool deserialize (vxldollar::stream &);
	void serialize_json (std::string &, bool = false) const override;
	void serialize_json (boost::property_tree::ptree &) const override;
	bool deserialize_json (boost::property_tree::ptree const &);
	void visit (vxldollar::block_visitor &) const override;
	void visit (vxldollar::mutable_block_visitor &) override;
	vxldollar::block_type type () const override;
	vxldollar::signature const & block_signature () const override;
	void signature_set (vxldollar::signature const &) override;
	bool operator== (vxldollar::block const &) const override;
	bool operator== (vxldollar::receive_block const &) const;
	bool valid_predecessor (vxldollar::block const &) const override;
	receive_hashables hashables;
	vxldollar::signature signature;
	uint64_t work;
	static std::size_t constexpr size = vxldollar::receive_hashables::size + sizeof (signature) + sizeof (work);
};
class open_hashables
{
public:
	open_hashables () = default;
	open_hashables (vxldollar::block_hash const &, vxldollar::account const &, vxldollar::account const &);
	open_hashables (bool &, vxldollar::stream &);
	open_hashables (bool &, boost::property_tree::ptree const &);
	void hash (blake2b_state &) const;
	vxldollar::block_hash source;
	vxldollar::account representative;
	vxldollar::account account;
	static std::size_t constexpr size = sizeof (source) + sizeof (representative) + sizeof (account);
};
class open_block : public vxldollar::block
{
public:
	open_block () = default;
	open_block (vxldollar::block_hash const &, vxldollar::account const &, vxldollar::account const &, vxldollar::raw_key const &, vxldollar::public_key const &, uint64_t);
	open_block (vxldollar::block_hash const &, vxldollar::account const &, vxldollar::account const &, std::nullptr_t);
	open_block (bool &, vxldollar::stream &);
	open_block (bool &, boost::property_tree::ptree const &);
	virtual ~open_block () = default;
	using vxldollar::block::hash;
	void hash (blake2b_state &) const override;
	uint64_t block_work () const override;
	void block_work_set (uint64_t) override;
	vxldollar::block_hash const & previous () const override;
	vxldollar::account const & account () const override;
	vxldollar::block_hash const & source () const override;
	vxldollar::root const & root () const override;
	vxldollar::account const & representative () const override;
	void serialize (vxldollar::stream &) const override;
	bool deserialize (vxldollar::stream &);
	void serialize_json (std::string &, bool = false) const override;
	void serialize_json (boost::property_tree::ptree &) const override;
	bool deserialize_json (boost::property_tree::ptree const &);
	void visit (vxldollar::block_visitor &) const override;
	void visit (vxldollar::mutable_block_visitor &) override;
	vxldollar::block_type type () const override;
	vxldollar::signature const & block_signature () const override;
	void signature_set (vxldollar::signature const &) override;
	bool operator== (vxldollar::block const &) const override;
	bool operator== (vxldollar::open_block const &) const;
	bool valid_predecessor (vxldollar::block const &) const override;
	vxldollar::open_hashables hashables;
	vxldollar::signature signature;
	uint64_t work;
	static std::size_t constexpr size = vxldollar::open_hashables::size + sizeof (signature) + sizeof (work);
};
class change_hashables
{
public:
	change_hashables () = default;
	change_hashables (vxldollar::block_hash const &, vxldollar::account const &);
	change_hashables (bool &, vxldollar::stream &);
	change_hashables (bool &, boost::property_tree::ptree const &);
	void hash (blake2b_state &) const;
	vxldollar::block_hash previous;
	vxldollar::account representative;
	static std::size_t constexpr size = sizeof (previous) + sizeof (representative);
};
class change_block : public vxldollar::block
{
public:
	change_block () = default;
	change_block (vxldollar::block_hash const &, vxldollar::account const &, vxldollar::raw_key const &, vxldollar::public_key const &, uint64_t);
	change_block (bool &, vxldollar::stream &);
	change_block (bool &, boost::property_tree::ptree const &);
	virtual ~change_block () = default;
	using vxldollar::block::hash;
	void hash (blake2b_state &) const override;
	uint64_t block_work () const override;
	void block_work_set (uint64_t) override;
	vxldollar::block_hash const & previous () const override;
	vxldollar::root const & root () const override;
	vxldollar::account const & representative () const override;
	void serialize (vxldollar::stream &) const override;
	bool deserialize (vxldollar::stream &);
	void serialize_json (std::string &, bool = false) const override;
	void serialize_json (boost::property_tree::ptree &) const override;
	bool deserialize_json (boost::property_tree::ptree const &);
	void visit (vxldollar::block_visitor &) const override;
	void visit (vxldollar::mutable_block_visitor &) override;
	vxldollar::block_type type () const override;
	vxldollar::signature const & block_signature () const override;
	void signature_set (vxldollar::signature const &) override;
	bool operator== (vxldollar::block const &) const override;
	bool operator== (vxldollar::change_block const &) const;
	bool valid_predecessor (vxldollar::block const &) const override;
	vxldollar::change_hashables hashables;
	vxldollar::signature signature;
	uint64_t work;
	static std::size_t constexpr size = vxldollar::change_hashables::size + sizeof (signature) + sizeof (work);
};
class state_hashables
{
public:
	state_hashables () = default;
	state_hashables (vxldollar::account const &, vxldollar::block_hash const &, vxldollar::account const &, vxldollar::amount const &, vxldollar::link const &);
	state_hashables (bool &, vxldollar::stream &);
	state_hashables (bool &, boost::property_tree::ptree const &);
	void hash (blake2b_state &) const;
	// Account# / public key that operates this account
	// Uses:
	// Bulk signature validation in advance of further ledger processing
	// Arranging uncomitted transactions by account
	vxldollar::account account;
	// Previous transaction in this chain
	vxldollar::block_hash previous;
	// Representative of this account
	vxldollar::account representative;
	// Current balance of this account
	// Allows lookup of account balance simply by looking at the head block
	vxldollar::amount balance;
	// Link field contains source block_hash if receiving, destination account if sending
	vxldollar::link link;
	// Serialized size
	static std::size_t constexpr size = sizeof (account) + sizeof (previous) + sizeof (representative) + sizeof (balance) + sizeof (link);
};
class state_block : public vxldollar::block
{
public:
	state_block () = default;
	state_block (vxldollar::account const &, vxldollar::block_hash const &, vxldollar::account const &, vxldollar::amount const &, vxldollar::link const &, vxldollar::raw_key const &, vxldollar::public_key const &, uint64_t);
	state_block (bool &, vxldollar::stream &);
	state_block (bool &, boost::property_tree::ptree const &);
	virtual ~state_block () = default;
	using vxldollar::block::hash;
	void hash (blake2b_state &) const override;
	uint64_t block_work () const override;
	void block_work_set (uint64_t) override;
	vxldollar::block_hash const & previous () const override;
	vxldollar::account const & account () const override;
	vxldollar::root const & root () const override;
	vxldollar::link const & link () const override;
	vxldollar::account const & representative () const override;
	vxldollar::amount const & balance () const override;
	void serialize (vxldollar::stream &) const override;
	bool deserialize (vxldollar::stream &);
	void serialize_json (std::string &, bool = false) const override;
	void serialize_json (boost::property_tree::ptree &) const override;
	bool deserialize_json (boost::property_tree::ptree const &);
	void visit (vxldollar::block_visitor &) const override;
	void visit (vxldollar::mutable_block_visitor &) override;
	vxldollar::block_type type () const override;
	vxldollar::signature const & block_signature () const override;
	void signature_set (vxldollar::signature const &) override;
	bool operator== (vxldollar::block const &) const override;
	bool operator== (vxldollar::state_block const &) const;
	bool valid_predecessor (vxldollar::block const &) const override;
	vxldollar::state_hashables hashables;
	vxldollar::signature signature;
	uint64_t work;
	static std::size_t constexpr size = vxldollar::state_hashables::size + sizeof (signature) + sizeof (work);
};
class block_visitor
{
public:
	virtual void send_block (vxldollar::send_block const &) = 0;
	virtual void receive_block (vxldollar::receive_block const &) = 0;
	virtual void open_block (vxldollar::open_block const &) = 0;
	virtual void change_block (vxldollar::change_block const &) = 0;
	virtual void state_block (vxldollar::state_block const &) = 0;
	virtual ~block_visitor () = default;
};
class mutable_block_visitor
{
public:
	virtual void send_block (vxldollar::send_block &) = 0;
	virtual void receive_block (vxldollar::receive_block &) = 0;
	virtual void open_block (vxldollar::open_block &) = 0;
	virtual void change_block (vxldollar::change_block &) = 0;
	virtual void state_block (vxldollar::state_block &) = 0;
	virtual ~mutable_block_visitor () = default;
};
/**
 * This class serves to find and return unique variants of a block in order to minimize memory usage
 */
class block_uniquer
{
public:
	using value_type = std::pair<vxldollar::uint256_union const, std::weak_ptr<vxldollar::block>>;

	std::shared_ptr<vxldollar::block> unique (std::shared_ptr<vxldollar::block> const &);
	size_t size ();

private:
	vxldollar::mutex mutex{ mutex_identifier (mutexes::block_uniquer) };
	std::unordered_map<std::remove_const_t<value_type::first_type>, value_type::second_type> blocks;
	static unsigned constexpr cleanup_count = 2;
};

std::unique_ptr<container_info_component> collect_container_info (block_uniquer & block_uniquer, std::string const & name);

std::shared_ptr<vxldollar::block> deserialize_block (vxldollar::stream &);
std::shared_ptr<vxldollar::block> deserialize_block (vxldollar::stream &, vxldollar::block_type, vxldollar::block_uniquer * = nullptr);
std::shared_ptr<vxldollar::block> deserialize_block_json (boost::property_tree::ptree const &, vxldollar::block_uniquer * = nullptr);
void serialize_block (vxldollar::stream &, vxldollar::block const &);
void block_memory_pool_purge ();
}
