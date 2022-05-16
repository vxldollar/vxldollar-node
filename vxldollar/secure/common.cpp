#include <vxldollar/crypto_lib/random_pool.hpp>
#include <vxldollar/lib/config.hpp>
#include <vxldollar/lib/numbers.hpp>
#include <vxldollar/lib/timer.hpp>
#include <vxldollar/secure/common.hpp>
#include <vxldollar/secure/store.hpp>

#include <crypto/cryptopp/words.h>

#include <boost/endian/conversion.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/variant/get.hpp>

#include <limits>
#include <queue>

#include <crypto/ed25519-donna/ed25519.h>

size_t constexpr vxldollar::send_block::size;
size_t constexpr vxldollar::receive_block::size;
size_t constexpr vxldollar::open_block::size;
size_t constexpr vxldollar::change_block::size;
size_t constexpr vxldollar::state_block::size;

vxldollar::networks vxldollar::network_constants::active_network = vxldollar::networks::ACTIVE_NETWORK;

namespace
{
//
char const * dev_private_key_data = "6b68b2404894270f3f47cf90fb25c13e9362bcab5dbc0facce899fbc2c869d35";
char const * dev_public_key_data = "399462C16AEF7FDBBE04AA7A5ED97AEFC66DFA0522322C7C8F49716139518312"; // vxld_1gened1pouuzugz1bcmtdueqouy8fqx1cajk7jyaykdje6wo51rki9z5kzcd
char const * beta_public_key_data = "39946278F3E6178B50FA70C82B05C3B2FAF2B58B70525C1AF122069D4EA56D2D"; // vxld_1genebwh9siqjfahnw8a7e4w9eqtyctrpw4kdifh4ai8mo9ccubf184tqx45
char const * live_public_key_data = "3994667291FB69EE28643F271ACA94A681580121E695FC42E952D854883A63C1"; // vxld_1genessb5yubxrn8ahs95d7bbbn3d11k5snozj3gknprck65nry3rmtjjjwk
std::string const test_public_key_data = vxldollar::get_env_or_default ("VXLDOLLAR_TEST_GENESIS_PUB", "399466B7128016D6815D3ADB103A2BE1F8E283742C30BE5D6CD0AE1696342197"); // vxld_1genetuj711ptt1otgpu41x4qrhrwc3qad3iqsgpsn7g4td5aaeq6e61e9tw
char const * dev_genesis_data = R"%%%({
	"type": "open",
    "source": "399462C16AEF7FDBBE04AA7A5ED97AEFC66DFA0522322C7C8F49716139518312",
    "representative": "vxld_1gened1pouuzugz1bcmtdueqouy8fqx1cajk7jyaykdje6wo51rki9z5kzcd",
    "account": "vxld_1gened1pouuzugz1bcmtdueqouy8fqx1cajk7jyaykdje6wo51rki9z5kzcd",
    "work": "379f75961a7d2d2f",
    "signature": "D6592B53BE346B929E31484FADEA516F671AF6E0C3E3C3BD84F109504237ECCECF3083F14037BA577B828204C9BCFA9F147B72CDD5317901ABC9C5389B2CE005"
    })%%%";

char const * beta_genesis_data = R"%%%({
	"type": "open",
    "source": "39946278F3E6178B50FA70C82B05C3B2FAF2B58B70525C1AF122069D4EA56D2D",
    "representative": "vxld_1genebwh9siqjfahnw8a7e4w9eqtyctrpw4kdifh4ai8mo9ccubf184tqx45",
    "account": "vxld_1genebwh9siqjfahnw8a7e4w9eqtyctrpw4kdifh4ai8mo9ccubf184tqx45",
    "work": "93ceaf996bfcd576",
    "signature": "1737D3067FB4587E687D00459835BD52A18019D0A884669EB62CB229E8CDDC0F35C489FFB90E6CFB7EE79AAB9F1083FBFD4A377ACE405F0B8A65BA83BB31C209"
    })%%%";

char const * live_genesis_data = R"%%%({
    "type": "open",
    "source": "3994667291FB69EE28643F271ACA94A681580121E695FC42E952D854883A63C1",
    "representative": "vxld_1genessb5yubxrn8ahs95d7bbbn3d11k5snozj3gknprck65nry3rmtjjjwk",
    "account": "vxld_1genessb5yubxrn8ahs95d7bbbn3d11k5snozj3gknprck65nry3rmtjjjwk",
    "work": "894e2987898bfa15",
    "signature": "7B5D60E8F783FCEF2EC08E55618F7D3C0429BBAF2ACA6C5DEF0F47EB9E315FDF7B1A8711DF1BDAEA70B0A0BFBA15E0553C67317BEDD49B3BE0A6C9EFE884CF06"
	})%%%";

std::string const test_genesis_data = vxldollar::get_env_or_default ("VXLDOLLAR_TEST_GENESIS_BLOCK", R"%%%({
	"type": "open",
    "source": "399466B7128016D6815D3ADB103A2BE1F8E283742C30BE5D6CD0AE1696342197",
    "representative": "vxld_1genetuj711ptt1otgpu41x4qrhrwc3qad3iqsgpsn7g4td5aaeq6e61e9tw",
    "account": "vxld_1genetuj711ptt1otgpu41x4qrhrwc3qad3iqsgpsn7g4td5aaeq6e61e9tw",
    "work": "73417bb9fbb77654",
    "signature": "5377A59A6C9F27CB0CAE6C486CC6FB0A8106FBB5722F0FC7A90FF582658C352EB24DDCDE18BCD5E0DCFA65CCC4C42259B33384412A75557C84E9C22D81C38E0C"
    })%%%");

std::shared_ptr<vxldollar::block> parse_block_from_genesis_data (std::string const & genesis_data_a)
{
	boost::property_tree::ptree tree;
	std::stringstream istream (genesis_data_a);
	boost::property_tree::read_json (istream, tree);
	return vxldollar::deserialize_block_json (tree);
}

//
char const * beta_canary_public_key_data = "A9144611FDD5B2E5B020D54DD846CA4210DEC3A5BD30B6B2B4FD7AAAF378CBA3"; // vxld_3canarazuofkwpr43ocfu35eniiiuu3tdhbiptsdbzdtodsqjkx56s7gmr4q
char const * live_canary_public_key_data = "A914463837967FBF29A5EDBFC0E2C034B5B06739D96523F617F4B23615D2E491"; // vxld_3canarw5h7mzqwntdufzr5je1f7op3mmmpd76hu3hx7k8rcx7s6jprpgzbdb
std::string const test_canary_public_key_data = vxldollar::get_env_or_default ("VXLDOLLAR_TEST_CANARY_PUB", "A914460276BC1E4811D2806B3D7CFF9A446B23444D3E13AED74656C4A2CDEFCC"); // vxld_3canar39fh1yb1ax715d9oyhz8k6fejnamby4gqfgjkprkjeuuyec8w4btuy
}

vxldollar::keypair vxldollar::dev::genesis_key{ dev_private_key_data };
vxldollar::network_params vxldollar::dev::network_params{ vxldollar::networks::vxldollar_dev_network };
vxldollar::ledger_constants & vxldollar::dev::constants{ vxldollar::dev::network_params.ledger };
std::shared_ptr<vxldollar::block> & vxldollar::dev::genesis = vxldollar::dev::constants.genesis;

vxldollar::network_params::network_params (vxldollar::networks network_a) :
	work{ network_a == vxldollar::networks::vxldollar_live_network ? vxldollar::work_thresholds::publish_full : network_a == vxldollar::networks::vxldollar_beta_network ? vxldollar::work_thresholds::publish_beta
		: network_a == vxldollar::networks::vxldollar_test_network                                                                                        ? vxldollar::work_thresholds::publish_test
																																				: vxldollar::work_thresholds::publish_dev },
	network{ work, network_a },
	ledger{ work, network_a },
	voting{ network },
	node{ network },
	portmapping{ network },
	bootstrap{ network }
{
	unsigned constexpr kdf_full_work = 64 * 1024;
	unsigned constexpr kdf_dev_work = 8;
	kdf_work = network.is_dev_network () ? kdf_dev_work : kdf_full_work;
}

vxldollar::ledger_constants::ledger_constants (vxldollar::work_thresholds & work, vxldollar::networks network_a) :
	work{ work },
	zero_key ("0"),
	vxldollar_beta_account (beta_public_key_data),
	vxldollar_live_account (live_public_key_data),
	vxldollar_test_account (test_public_key_data),
	vxldollar_dev_genesis (parse_block_from_genesis_data (dev_genesis_data)),
	vxldollar_beta_genesis (parse_block_from_genesis_data (beta_genesis_data)),
	vxldollar_live_genesis (parse_block_from_genesis_data (live_genesis_data)),
	vxldollar_test_genesis (parse_block_from_genesis_data (test_genesis_data)),
	genesis (network_a == vxldollar::networks::vxldollar_dev_network ? vxldollar_dev_genesis : network_a == vxldollar::networks::vxldollar_beta_network ? vxldollar_beta_genesis
	: network_a == vxldollar::networks::vxldollar_test_network                                                                           ? vxldollar_test_genesis
																															   : vxldollar_live_genesis),
	genesis_amount{ std::numeric_limits<vxldollar::uint128_t>::max () },
	burn_account{},
	vxldollar_dev_final_votes_canary_account (dev_public_key_data),
	vxldollar_beta_final_votes_canary_account (beta_canary_public_key_data),
	vxldollar_live_final_votes_canary_account (live_canary_public_key_data),
	vxldollar_test_final_votes_canary_account (test_canary_public_key_data),
	final_votes_canary_account (network_a == vxldollar::networks::vxldollar_dev_network ? vxldollar_dev_final_votes_canary_account : network_a == vxldollar::networks::vxldollar_beta_network ? vxldollar_beta_final_votes_canary_account
	: network_a == vxldollar::networks::vxldollar_test_network                                                                                                                 ? vxldollar_test_final_votes_canary_account
																																									 : vxldollar_live_final_votes_canary_account),
	vxldollar_dev_final_votes_canary_height (1),
	vxldollar_beta_final_votes_canary_height (1),
	vxldollar_live_final_votes_canary_height (1),
	vxldollar_test_final_votes_canary_height (1),
	final_votes_canary_height (network_a == vxldollar::networks::vxldollar_dev_network ? vxldollar_dev_final_votes_canary_height : network_a == vxldollar::networks::vxldollar_beta_network ? vxldollar_beta_final_votes_canary_height
	: network_a == vxldollar::networks::vxldollar_test_network                                                                                                               ? vxldollar_test_final_votes_canary_height
																																								   : vxldollar_live_final_votes_canary_height)
{
	vxldollar_beta_genesis->sideband_set (vxldollar::block_sideband (vxldollar_beta_genesis->account (), 0, std::numeric_limits<vxldollar::uint128_t>::max (), 1, vxldollar::seconds_since_epoch (), vxldollar::epoch::epoch_0, false, false, false, vxldollar::epoch::epoch_0));
	vxldollar_dev_genesis->sideband_set (vxldollar::block_sideband (vxldollar_dev_genesis->account (), 0, std::numeric_limits<vxldollar::uint128_t>::max (), 1, vxldollar::seconds_since_epoch (), vxldollar::epoch::epoch_0, false, false, false, vxldollar::epoch::epoch_0));
	vxldollar_live_genesis->sideband_set (vxldollar::block_sideband (vxldollar_live_genesis->account (), 0, std::numeric_limits<vxldollar::uint128_t>::max (), 1, vxldollar::seconds_since_epoch (), vxldollar::epoch::epoch_0, false, false, false, vxldollar::epoch::epoch_0));
	vxldollar_test_genesis->sideband_set (vxldollar::block_sideband (vxldollar_test_genesis->account (), 0, std::numeric_limits<vxldollar::uint128_t>::max (), 1, vxldollar::seconds_since_epoch (), vxldollar::epoch::epoch_0, false, false, false, vxldollar::epoch::epoch_0));

	vxldollar::link epoch_link_v1;
	char const * epoch_message_v1 ("epoch v1 block");
	strncpy ((char *)epoch_link_v1.bytes.data (), epoch_message_v1, epoch_link_v1.bytes.size ());
	epochs.add (vxldollar::epoch::epoch_1, genesis->account (), epoch_link_v1);

	//
	vxldollar::link epoch_link_v2;
	vxldollar::account vxldollar_live_epoch_v2_signer;
	auto error (vxldollar_live_epoch_v2_signer.decode_account ("vxld_3epochdg4gwpu4qxoa61oepnggbtc4tu1xirjhd48ckd5pdcazfqx1wxgx9i"));
	debug_assert (!error);
	auto epoch_v2_signer (network_a == vxldollar::networks::vxldollar_dev_network ? vxldollar::dev::genesis_key.pub : network_a == vxldollar::networks::vxldollar_beta_network ? vxldollar_beta_account
	: network_a == vxldollar::networks::vxldollar_test_network                                                                                                  ? vxldollar_test_account
																																					  : vxldollar_live_epoch_v2_signer);
	char const * epoch_message_v2 ("epoch v2 block");
	strncpy ((char *)epoch_link_v2.bytes.data (), epoch_message_v2, epoch_link_v2.bytes.size ());
	epochs.add (vxldollar::epoch::epoch_2, epoch_v2_signer, epoch_link_v2);
}

vxldollar::hardened_constants & vxldollar::hardened_constants::get ()
{
	static hardened_constants instance{};
	return instance;
}

vxldollar::hardened_constants::hardened_constants () :
	not_an_account{},
	random_128{}
{
	vxldollar::random_pool::generate_block (not_an_account.bytes.data (), not_an_account.bytes.size ());
	vxldollar::random_pool::generate_block (random_128.bytes.data (), random_128.bytes.size ());
}

vxldollar::node_constants::node_constants (vxldollar::network_constants & network_constants)
{
	backup_interval = std::chrono::minutes (5);
	search_pending_interval = network_constants.is_dev_network () ? std::chrono::seconds (1) : std::chrono::seconds (5 * 60);
	unchecked_cleaning_interval = std::chrono::minutes (30);
	process_confirmed_interval = network_constants.is_dev_network () ? std::chrono::milliseconds (50) : std::chrono::milliseconds (500);
	max_weight_samples = (network_constants.is_live_network () || network_constants.is_test_network ()) ? 4032 : 288;
	weight_period = 5 * 60; // 5 minutes
}

vxldollar::voting_constants::voting_constants (vxldollar::network_constants & network_constants) :
	max_cache{ network_constants.is_dev_network () ? 256U : 128U * 1024 },
	delay{ network_constants.is_dev_network () ? 1 : 15 }
{
}

vxldollar::portmapping_constants::portmapping_constants (vxldollar::network_constants & network_constants)
{
	lease_duration = std::chrono::seconds (1787); // ~30 minutes
	health_check_period = std::chrono::seconds (53);
}

vxldollar::bootstrap_constants::bootstrap_constants (vxldollar::network_constants & network_constants)
{
	lazy_max_pull_blocks = network_constants.is_dev_network () ? 2 : 512;
	lazy_min_pull_blocks = network_constants.is_dev_network () ? 1 : 32;
	frontier_retry_limit = network_constants.is_dev_network () ? 2 : 16;
	lazy_retry_limit = network_constants.is_dev_network () ? 2 : frontier_retry_limit * 4;
	lazy_destinations_retry_limit = network_constants.is_dev_network () ? 1 : frontier_retry_limit / 4;
	gap_cache_bootstrap_start_interval = network_constants.is_dev_network () ? std::chrono::milliseconds (5) : std::chrono::milliseconds (30 * 1000);
	default_frontiers_age_seconds = network_constants.is_dev_network () ? 1 : 24 * 60 * 60; // 1 second for dev network, 24 hours for live/beta
}

// Create a new random keypair
vxldollar::keypair::keypair ()
{
	random_pool::generate_block (prv.bytes.data (), prv.bytes.size ());
	ed25519_publickey (prv.bytes.data (), pub.bytes.data ());
}

// Create a keypair given a private key
vxldollar::keypair::keypair (vxldollar::raw_key && prv_a) :
	prv (std::move (prv_a))
{
	ed25519_publickey (prv.bytes.data (), pub.bytes.data ());
}

// Create a keypair given a hex string of the private key
vxldollar::keypair::keypair (std::string const & prv_a)
{
	[[maybe_unused]] auto error (prv.decode_hex (prv_a));
	debug_assert (!error);
	ed25519_publickey (prv.bytes.data (), pub.bytes.data ());
}

// Serialize a block prefixed with an 8-bit typecode
void vxldollar::serialize_block (vxldollar::stream & stream_a, vxldollar::block const & block_a)
{
	write (stream_a, block_a.type ());
	block_a.serialize (stream_a);
}

vxldollar::account_info::account_info (vxldollar::block_hash const & head_a, vxldollar::account const & representative_a, vxldollar::block_hash const & open_block_a, vxldollar::amount const & balance_a, uint64_t modified_a, uint64_t block_count_a, vxldollar::epoch epoch_a) :
	head (head_a),
	representative (representative_a),
	open_block (open_block_a),
	balance (balance_a),
	modified (modified_a),
	block_count (block_count_a),
	epoch_m (epoch_a)
{
}

bool vxldollar::account_info::deserialize (vxldollar::stream & stream_a)
{
	auto error (false);
	try
	{
		vxldollar::read (stream_a, head.bytes);
		vxldollar::read (stream_a, representative.bytes);
		vxldollar::read (stream_a, open_block.bytes);
		vxldollar::read (stream_a, balance.bytes);
		vxldollar::read (stream_a, modified);
		vxldollar::read (stream_a, block_count);
		vxldollar::read (stream_a, epoch_m);
	}
	catch (std::runtime_error const &)
	{
		error = true;
	}

	return error;
}

bool vxldollar::account_info::operator== (vxldollar::account_info const & other_a) const
{
	return head == other_a.head && representative == other_a.representative && open_block == other_a.open_block && balance == other_a.balance && modified == other_a.modified && block_count == other_a.block_count && epoch () == other_a.epoch ();
}

bool vxldollar::account_info::operator!= (vxldollar::account_info const & other_a) const
{
	return !(*this == other_a);
}

size_t vxldollar::account_info::db_size () const
{
	debug_assert (reinterpret_cast<uint8_t const *> (this) == reinterpret_cast<uint8_t const *> (&head));
	debug_assert (reinterpret_cast<uint8_t const *> (&head) + sizeof (head) == reinterpret_cast<uint8_t const *> (&representative));
	debug_assert (reinterpret_cast<uint8_t const *> (&representative) + sizeof (representative) == reinterpret_cast<uint8_t const *> (&open_block));
	debug_assert (reinterpret_cast<uint8_t const *> (&open_block) + sizeof (open_block) == reinterpret_cast<uint8_t const *> (&balance));
	debug_assert (reinterpret_cast<uint8_t const *> (&balance) + sizeof (balance) == reinterpret_cast<uint8_t const *> (&modified));
	debug_assert (reinterpret_cast<uint8_t const *> (&modified) + sizeof (modified) == reinterpret_cast<uint8_t const *> (&block_count));
	debug_assert (reinterpret_cast<uint8_t const *> (&block_count) + sizeof (block_count) == reinterpret_cast<uint8_t const *> (&epoch_m));
	return sizeof (head) + sizeof (representative) + sizeof (open_block) + sizeof (balance) + sizeof (modified) + sizeof (block_count) + sizeof (epoch_m);
}

vxldollar::epoch vxldollar::account_info::epoch () const
{
	return epoch_m;
}

vxldollar::pending_info::pending_info (vxldollar::account const & source_a, vxldollar::amount const & amount_a, vxldollar::epoch epoch_a) :
	source (source_a),
	amount (amount_a),
	epoch (epoch_a)
{
}

bool vxldollar::pending_info::deserialize (vxldollar::stream & stream_a)
{
	auto error (false);
	try
	{
		vxldollar::read (stream_a, source.bytes);
		vxldollar::read (stream_a, amount.bytes);
		vxldollar::read (stream_a, epoch);
	}
	catch (std::runtime_error const &)
	{
		error = true;
	}

	return error;
}

size_t vxldollar::pending_info::db_size () const
{
	return sizeof (source) + sizeof (amount) + sizeof (epoch);
}

bool vxldollar::pending_info::operator== (vxldollar::pending_info const & other_a) const
{
	return source == other_a.source && amount == other_a.amount && epoch == other_a.epoch;
}

vxldollar::pending_key::pending_key (vxldollar::account const & account_a, vxldollar::block_hash const & hash_a) :
	account (account_a),
	hash (hash_a)
{
}

bool vxldollar::pending_key::deserialize (vxldollar::stream & stream_a)
{
	auto error (false);
	try
	{
		vxldollar::read (stream_a, account.bytes);
		vxldollar::read (stream_a, hash.bytes);
	}
	catch (std::runtime_error const &)
	{
		error = true;
	}

	return error;
}

bool vxldollar::pending_key::operator== (vxldollar::pending_key const & other_a) const
{
	return account == other_a.account && hash == other_a.hash;
}

vxldollar::account const & vxldollar::pending_key::key () const
{
	return account;
}

vxldollar::unchecked_info::unchecked_info (std::shared_ptr<vxldollar::block> const & block_a, vxldollar::account const & account_a, vxldollar::signature_verification verified_a) :
	block (block_a),
	account (account_a),
	modified_m (vxldollar::seconds_since_epoch ()),
	verified (verified_a)
{
}

vxldollar::unchecked_info::unchecked_info (std::shared_ptr<vxldollar::block> const & block) :
	unchecked_info{ block, block->account (), vxldollar::signature_verification::unknown }
{
}

void vxldollar::unchecked_info::serialize (vxldollar::stream & stream_a) const
{
	debug_assert (block != nullptr);
	vxldollar::serialize_block (stream_a, *block);
	vxldollar::write (stream_a, account.bytes);
	vxldollar::write (stream_a, modified_m);
	vxldollar::write (stream_a, verified);
}

bool vxldollar::unchecked_info::deserialize (vxldollar::stream & stream_a)
{
	block = vxldollar::deserialize_block (stream_a);
	bool error (block == nullptr);
	if (!error)
	{
		try
		{
			vxldollar::read (stream_a, account.bytes);
			vxldollar::read (stream_a, modified_m);
			vxldollar::read (stream_a, verified);
		}
		catch (std::runtime_error const &)
		{
			error = true;
		}
	}
	return error;
}

uint64_t vxldollar::unchecked_info::modified () const
{
	return modified_m;
}

vxldollar::endpoint_key::endpoint_key (std::array<uint8_t, 16> const & address_a, uint16_t port_a) :
	address (address_a), network_port (boost::endian::native_to_big (port_a))
{
}

std::array<uint8_t, 16> const & vxldollar::endpoint_key::address_bytes () const
{
	return address;
}

uint16_t vxldollar::endpoint_key::port () const
{
	return boost::endian::big_to_native (network_port);
}

vxldollar::confirmation_height_info::confirmation_height_info (uint64_t confirmation_height_a, vxldollar::block_hash const & confirmed_frontier_a) :
	height (confirmation_height_a),
	frontier (confirmed_frontier_a)
{
}

void vxldollar::confirmation_height_info::serialize (vxldollar::stream & stream_a) const
{
	vxldollar::write (stream_a, height);
	vxldollar::write (stream_a, frontier);
}

bool vxldollar::confirmation_height_info::deserialize (vxldollar::stream & stream_a)
{
	auto error (false);
	try
	{
		vxldollar::read (stream_a, height);
		vxldollar::read (stream_a, frontier);
	}
	catch (std::runtime_error const &)
	{
		error = true;
	}
	return error;
}

vxldollar::block_info::block_info (vxldollar::account const & account_a, vxldollar::amount const & balance_a) :
	account (account_a),
	balance (balance_a)
{
}

bool vxldollar::vote::operator== (vxldollar::vote const & other_a) const
{
	auto blocks_equal (true);
	if (blocks.size () != other_a.blocks.size ())
	{
		blocks_equal = false;
	}
	else
	{
		for (auto i (0); blocks_equal && i < blocks.size (); ++i)
		{
			auto block (blocks[i]);
			auto other_block (other_a.blocks[i]);
			if (block.which () != other_block.which ())
			{
				blocks_equal = false;
			}
			else if (block.which ())
			{
				if (boost::get<vxldollar::block_hash> (block) != boost::get<vxldollar::block_hash> (other_block))
				{
					blocks_equal = false;
				}
			}
			else
			{
				if (!(*boost::get<std::shared_ptr<vxldollar::block>> (block) == *boost::get<std::shared_ptr<vxldollar::block>> (other_block)))
				{
					blocks_equal = false;
				}
			}
		}
	}
	return timestamp_m == other_a.timestamp_m && blocks_equal && account == other_a.account && signature == other_a.signature;
}

bool vxldollar::vote::operator!= (vxldollar::vote const & other_a) const
{
	return !(*this == other_a);
}

void vxldollar::vote::serialize_json (boost::property_tree::ptree & tree) const
{
	tree.put ("account", account.to_account ());
	tree.put ("signature", signature.number ());
	tree.put ("sequence", std::to_string (timestamp ()));
	tree.put ("timestamp", std::to_string (timestamp ()));
	tree.put ("duration", std::to_string (duration_bits ()));
	boost::property_tree::ptree blocks_tree;
	for (auto block : blocks)
	{
		boost::property_tree::ptree entry;
		if (block.which ())
		{
			entry.put ("", boost::get<vxldollar::block_hash> (block).to_string ());
		}
		else
		{
			entry.put ("", boost::get<std::shared_ptr<vxldollar::block>> (block)->hash ().to_string ());
		}
		blocks_tree.push_back (std::make_pair ("", entry));
	}
	tree.add_child ("blocks", blocks_tree);
}

std::string vxldollar::vote::to_json () const
{
	std::stringstream stream;
	boost::property_tree::ptree tree;
	serialize_json (tree);
	boost::property_tree::write_json (stream, tree);
	return stream.str ();
}

/**
 * Returns the timestamp of the vote (with the duration bits masked, set to zero)
 * If it is a final vote, all the bits including duration bits are returned as they are, all FF
 */
uint64_t vxldollar::vote::timestamp () const
{
	return (timestamp_m == std::numeric_limits<uint64_t>::max ())
	? timestamp_m // final vote
	: (timestamp_m & timestamp_mask);
}

uint8_t vxldollar::vote::duration_bits () const
{
	// Duration field is specified in the 4 low-order bits of the timestamp.
	// This makes the timestamp have a minimum granularity of 16ms
	// The duration is specified as 2^(duration + 4) giving it a range of 16-524,288ms in power of two increments
	auto result = timestamp_m & ~timestamp_mask;
	debug_assert (result < 16);
	return static_cast<uint8_t> (result);
}

std::chrono::milliseconds vxldollar::vote::duration () const
{
	return std::chrono::milliseconds{ 1u << (duration_bits () + 4) };
}

vxldollar::vote::vote (vxldollar::vote const & other_a) :
	timestamp_m{ other_a.timestamp_m },
	blocks (other_a.blocks),
	account (other_a.account),
	signature (other_a.signature)
{
}

vxldollar::vote::vote (bool & error_a, vxldollar::stream & stream_a, vxldollar::block_uniquer * uniquer_a)
{
	error_a = deserialize (stream_a, uniquer_a);
}

vxldollar::vote::vote (bool & error_a, vxldollar::stream & stream_a, vxldollar::block_type type_a, vxldollar::block_uniquer * uniquer_a)
{
	try
	{
		vxldollar::read (stream_a, account.bytes);
		vxldollar::read (stream_a, signature.bytes);
		vxldollar::read (stream_a, timestamp_m);

		while (stream_a.in_avail () > 0)
		{
			if (type_a == vxldollar::block_type::not_a_block)
			{
				vxldollar::block_hash block_hash;
				vxldollar::read (stream_a, block_hash);
				blocks.push_back (block_hash);
			}
			else
			{
				auto block (vxldollar::deserialize_block (stream_a, type_a, uniquer_a));
				if (block == nullptr)
				{
					throw std::runtime_error ("Block is null");
				}
				blocks.push_back (block);
			}
		}
	}
	catch (std::runtime_error const &)
	{
		error_a = true;
	}

	if (blocks.empty ())
	{
		error_a = true;
	}
}

vxldollar::vote::vote (vxldollar::account const & account_a, vxldollar::raw_key const & prv_a, uint64_t timestamp_a, uint8_t duration, std::shared_ptr<vxldollar::block> const & block_a) :
	timestamp_m{ packed_timestamp (timestamp_a, duration) },
	blocks (1, block_a),
	account (account_a),
	signature (vxldollar::sign_message (prv_a, account_a, hash ()))
{
}

vxldollar::vote::vote (vxldollar::account const & account_a, vxldollar::raw_key const & prv_a, uint64_t timestamp_a, uint8_t duration, std::vector<vxldollar::block_hash> const & blocks_a) :
	timestamp_m{ packed_timestamp (timestamp_a, duration) },
	account (account_a)
{
	debug_assert (!blocks_a.empty ());
	debug_assert (blocks_a.size () <= 12);
	blocks.reserve (blocks_a.size ());
	std::copy (blocks_a.cbegin (), blocks_a.cend (), std::back_inserter (blocks));
	signature = vxldollar::sign_message (prv_a, account_a, hash ());
}

std::string vxldollar::vote::hashes_string () const
{
	std::string result;
	for (auto hash : *this)
	{
		result += hash.to_string ();
		result += ", ";
	}
	return result;
}

std::string const vxldollar::vote::hash_prefix = "vote ";

vxldollar::block_hash vxldollar::vote::hash () const
{
	vxldollar::block_hash result;
	blake2b_state hash;
	blake2b_init (&hash, sizeof (result.bytes));
	if (blocks.size () > 1 || (!blocks.empty () && blocks.front ().which ()))
	{
		blake2b_update (&hash, hash_prefix.data (), hash_prefix.size ());
	}
	for (auto block_hash : *this)
	{
		blake2b_update (&hash, block_hash.bytes.data (), sizeof (block_hash.bytes));
	}
	union
	{
		uint64_t qword;
		std::array<uint8_t, 8> bytes;
	};
	qword = timestamp_m;
	blake2b_update (&hash, bytes.data (), sizeof (bytes));
	blake2b_final (&hash, result.bytes.data (), sizeof (result.bytes));
	return result;
}

vxldollar::block_hash vxldollar::vote::full_hash () const
{
	vxldollar::block_hash result;
	blake2b_state state;
	blake2b_init (&state, sizeof (result.bytes));
	blake2b_update (&state, hash ().bytes.data (), sizeof (hash ().bytes));
	blake2b_update (&state, account.bytes.data (), sizeof (account.bytes.data ()));
	blake2b_update (&state, signature.bytes.data (), sizeof (signature.bytes.data ()));
	blake2b_final (&state, result.bytes.data (), sizeof (result.bytes));
	return result;
}

void vxldollar::vote::serialize (vxldollar::stream & stream_a, vxldollar::block_type type) const
{
	write (stream_a, account);
	write (stream_a, signature);
	write (stream_a, boost::endian::native_to_little (timestamp_m));
	for (auto const & block : blocks)
	{
		if (block.which ())
		{
			debug_assert (type == vxldollar::block_type::not_a_block);
			write (stream_a, boost::get<vxldollar::block_hash> (block));
		}
		else
		{
			if (type == vxldollar::block_type::not_a_block)
			{
				write (stream_a, boost::get<std::shared_ptr<vxldollar::block>> (block)->hash ());
			}
			else
			{
				boost::get<std::shared_ptr<vxldollar::block>> (block)->serialize (stream_a);
			}
		}
	}
}

void vxldollar::vote::serialize (vxldollar::stream & stream_a) const
{
	write (stream_a, account);
	write (stream_a, signature);
	write (stream_a, boost::endian::native_to_little (timestamp_m));
	for (auto const & block : blocks)
	{
		if (block.which ())
		{
			write (stream_a, vxldollar::block_type::not_a_block);
			write (stream_a, boost::get<vxldollar::block_hash> (block));
		}
		else
		{
			vxldollar::serialize_block (stream_a, *boost::get<std::shared_ptr<vxldollar::block>> (block));
		}
	}
}

bool vxldollar::vote::deserialize (vxldollar::stream & stream_a, vxldollar::block_uniquer * uniquer_a)
{
	auto error (false);
	try
	{
		vxldollar::read (stream_a, account);
		vxldollar::read (stream_a, signature);
		vxldollar::read (stream_a, timestamp_m);
		boost::endian::little_to_native_inplace (timestamp_m);

		vxldollar::block_type type;

		while (true)
		{
			if (vxldollar::try_read (stream_a, type))
			{
				// Reached the end of the stream
				break;
			}

			if (type == vxldollar::block_type::not_a_block)
			{
				vxldollar::block_hash block_hash;
				vxldollar::read (stream_a, block_hash);
				blocks.push_back (block_hash);
			}
			else
			{
				auto block (vxldollar::deserialize_block (stream_a, type, uniquer_a));
				if (block == nullptr)
				{
					throw std::runtime_error ("Block is empty");
				}

				blocks.push_back (block);
			}
		}
	}
	catch (std::runtime_error const &)
	{
		error = true;
	}

	if (blocks.empty ())
	{
		error = true;
	}

	return error;
}

bool vxldollar::vote::validate () const
{
	return vxldollar::validate_message (account, hash (), signature);
}

uint64_t vxldollar::vote::packed_timestamp (uint64_t timestamp, uint8_t duration) const
{
	debug_assert (duration <= duration_max && "Invalid duration");
	debug_assert ((!(timestamp == timestamp_max) || (duration == duration_max)) && "Invalid final vote");
	return (timestamp & timestamp_mask) | duration;
}

vxldollar::block_hash vxldollar::iterate_vote_blocks_as_hash::operator() (boost::variant<std::shared_ptr<vxldollar::block>, vxldollar::block_hash> const & item) const
{
	vxldollar::block_hash result;
	if (item.which ())
	{
		result = boost::get<vxldollar::block_hash> (item);
	}
	else
	{
		result = boost::get<std::shared_ptr<vxldollar::block>> (item)->hash ();
	}
	return result;
}

boost::transform_iterator<vxldollar::iterate_vote_blocks_as_hash, vxldollar::vote_blocks_vec_iter> vxldollar::vote::begin () const
{
	return boost::transform_iterator<vxldollar::iterate_vote_blocks_as_hash, vxldollar::vote_blocks_vec_iter> (blocks.begin (), vxldollar::iterate_vote_blocks_as_hash ());
}

boost::transform_iterator<vxldollar::iterate_vote_blocks_as_hash, vxldollar::vote_blocks_vec_iter> vxldollar::vote::end () const
{
	return boost::transform_iterator<vxldollar::iterate_vote_blocks_as_hash, vxldollar::vote_blocks_vec_iter> (blocks.end (), vxldollar::iterate_vote_blocks_as_hash ());
}

vxldollar::vote_uniquer::vote_uniquer (vxldollar::block_uniquer & uniquer_a) :
	uniquer (uniquer_a)
{
}

std::shared_ptr<vxldollar::vote> vxldollar::vote_uniquer::unique (std::shared_ptr<vxldollar::vote> const & vote_a)
{
	auto result (vote_a);
	if (result != nullptr && !result->blocks.empty ())
	{
		if (!result->blocks.front ().which ())
		{
			result->blocks.front () = uniquer.unique (boost::get<std::shared_ptr<vxldollar::block>> (result->blocks.front ()));
		}
		vxldollar::block_hash key (vote_a->full_hash ());
		vxldollar::lock_guard<vxldollar::mutex> lock (mutex);
		auto & existing (votes[key]);
		if (auto block_l = existing.lock ())
		{
			result = block_l;
		}
		else
		{
			existing = vote_a;
		}

		release_assert (std::numeric_limits<CryptoPP::word32>::max () > votes.size ());
		for (auto i (0); i < cleanup_count && !votes.empty (); ++i)
		{
			auto random_offset = vxldollar::random_pool::generate_word32 (0, static_cast<CryptoPP::word32> (votes.size () - 1));

			auto existing (std::next (votes.begin (), random_offset));
			if (existing == votes.end ())
			{
				existing = votes.begin ();
			}
			if (existing != votes.end ())
			{
				if (auto block_l = existing->second.lock ())
				{
					// Still live
				}
				else
				{
					votes.erase (existing);
				}
			}
		}
	}
	return result;
}

size_t vxldollar::vote_uniquer::size ()
{
	vxldollar::lock_guard<vxldollar::mutex> lock (mutex);
	return votes.size ();
}

std::unique_ptr<vxldollar::container_info_component> vxldollar::collect_container_info (vote_uniquer & vote_uniquer, std::string const & name)
{
	auto count = vote_uniquer.size ();
	auto sizeof_element = sizeof (vote_uniquer::value_type);
	auto composite = std::make_unique<container_info_composite> (name);
	composite->add_component (std::make_unique<container_info_leaf> (container_info{ "votes", count, sizeof_element }));
	return composite;
}

vxldollar::wallet_id vxldollar::random_wallet_id ()
{
	vxldollar::wallet_id wallet_id;
	vxldollar::uint256_union dummy_secret;
	random_pool::generate_block (dummy_secret.bytes.data (), dummy_secret.bytes.size ());
	ed25519_publickey (dummy_secret.bytes.data (), wallet_id.bytes.data ());
	return wallet_id;
}

vxldollar::unchecked_key::unchecked_key (vxldollar::hash_or_account const & dependency) :
	unchecked_key{ dependency, 0 }
{
}

vxldollar::unchecked_key::unchecked_key (vxldollar::hash_or_account const & previous_a, vxldollar::block_hash const & hash_a) :
	previous (previous_a.hash),
	hash (hash_a)
{
}

vxldollar::unchecked_key::unchecked_key (vxldollar::uint512_union const & union_a) :
	previous (union_a.uint256s[0].number ()),
	hash (union_a.uint256s[1].number ())
{
}

bool vxldollar::unchecked_key::deserialize (vxldollar::stream & stream_a)
{
	auto error (false);
	try
	{
		vxldollar::read (stream_a, previous.bytes);
		vxldollar::read (stream_a, hash.bytes);
	}
	catch (std::runtime_error const &)
	{
		error = true;
	}

	return error;
}

bool vxldollar::unchecked_key::operator== (vxldollar::unchecked_key const & other_a) const
{
	return previous == other_a.previous && hash == other_a.hash;
}

vxldollar::block_hash const & vxldollar::unchecked_key::key () const
{
	return previous;
}

void vxldollar::generate_cache::enable_all ()
{
	reps = true;
	cemented_count = true;
	unchecked_count = true;
	account_count = true;
}
