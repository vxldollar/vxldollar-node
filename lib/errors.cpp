#include <vxldollar/lib/errors.hpp>
#include <vxldollar/lib/utility.hpp>

#include <boost/system/error_code.hpp>

std::string vxldollar::error_common_messages::message (int ev) const
{
	switch (static_cast<vxldollar::error_common> (ev))
	{
		case vxldollar::error_common::generic:
			return "Unknown error";
		case vxldollar::error_common::access_denied:
			return "Access denied";
		case vxldollar::error_common::missing_account:
			return "Missing account";
		case vxldollar::error_common::missing_balance:
			return "Missing balance";
		case vxldollar::error_common::missing_link:
			return "Missing link, source or destination";
		case vxldollar::error_common::missing_previous:
			return "Missing previous";
		case vxldollar::error_common::missing_representative:
			return "Missing representative";
		case vxldollar::error_common::missing_signature:
			return "Missing signature";
		case vxldollar::error_common::missing_work:
			return "Missing work";
		case vxldollar::error_common::exception:
			return "Exception thrown";
		case vxldollar::error_common::account_exists:
			return "Account already exists";
		case vxldollar::error_common::account_not_found:
			return "Account not found";
		case vxldollar::error_common::account_not_found_wallet:
			return "Account not found in wallet";
		case vxldollar::error_common::bad_account_number:
			return "Bad account number";
		case vxldollar::error_common::bad_balance:
			return "Bad balance";
		case vxldollar::error_common::bad_link:
			return "Bad link value";
		case vxldollar::error_common::bad_previous:
			return "Bad previous hash";
		case vxldollar::error_common::bad_representative_number:
			return "Bad representative";
		case vxldollar::error_common::bad_source:
			return "Bad source";
		case vxldollar::error_common::bad_signature:
			return "Bad signature";
		case vxldollar::error_common::bad_private_key:
			return "Bad private key";
		case vxldollar::error_common::bad_public_key:
			return "Bad public key";
		case vxldollar::error_common::bad_seed:
			return "Bad seed";
		case vxldollar::error_common::bad_threshold:
			return "Bad threshold number";
		case vxldollar::error_common::bad_wallet_number:
			return "Bad wallet number";
		case vxldollar::error_common::bad_work_format:
			return "Bad work";
		case vxldollar::error_common::disabled_local_work_generation:
			return "Local work generation is disabled";
		case vxldollar::error_common::disabled_work_generation:
			return "Work generation is disabled";
		case vxldollar::error_common::failure_work_generation:
			return "Work generation cancellation or failure";
		case vxldollar::error_common::insufficient_balance:
			return "Insufficient balance";
		case vxldollar::error_common::invalid_amount:
			return "Invalid amount number";
		case vxldollar::error_common::invalid_amount_big:
			return "Amount too big";
		case vxldollar::error_common::invalid_count:
			return "Invalid count";
		case vxldollar::error_common::invalid_ip_address:
			return "Invalid IP address";
		case vxldollar::error_common::invalid_port:
			return "Invalid port";
		case vxldollar::error_common::invalid_index:
			return "Invalid index";
		case vxldollar::error_common::invalid_type_conversion:
			return "Invalid type conversion";
		case vxldollar::error_common::invalid_work:
			return "Invalid work";
		case vxldollar::error_common::is_not_state_block:
			return "Must be a state block";
		case vxldollar::error_common::numeric_conversion:
			return "Numeric conversion error";
		case vxldollar::error_common::tracking_not_enabled:
			return "Database transaction tracking is not enabled in the config";
		case vxldollar::error_common::wallet_lmdb_max_dbs:
			return "Failed to create wallet. Increase lmdb_max_dbs in node config";
		case vxldollar::error_common::wallet_locked:
			return "Wallet is locked";
		case vxldollar::error_common::wallet_not_found:
			return "Wallet not found";
	}

	return "Invalid error code";
}

std::string vxldollar::error_blocks_messages::message (int ev) const
{
	switch (static_cast<vxldollar::error_blocks> (ev))
	{
		case vxldollar::error_blocks::generic:
			return "Unknown error";
		case vxldollar::error_blocks::bad_hash_number:
			return "Bad hash number";
		case vxldollar::error_blocks::invalid_block:
			return "Block is invalid";
		case vxldollar::error_blocks::invalid_block_hash:
			return "Invalid block hash";
		case vxldollar::error_blocks::invalid_type:
			return "Invalid block type";
		case vxldollar::error_blocks::not_found:
			return "Block not found";
		case vxldollar::error_blocks::work_low:
			return "Block work is less than threshold";
	}

	return "Invalid error code";
}

std::string vxldollar::error_rpc_messages::message (int ev) const
{
	switch (static_cast<vxldollar::error_rpc> (ev))
	{
		case vxldollar::error_rpc::generic:
			return "Unknown error";
		case vxldollar::error_rpc::empty_response:
			return "Empty response";
		case vxldollar::error_rpc::bad_destination:
			return "Bad destination account";
		case vxldollar::error_rpc::bad_difficulty_format:
			return "Bad difficulty";
		case vxldollar::error_rpc::bad_key:
			return "Bad key";
		case vxldollar::error_rpc::bad_link:
			return "Bad link number";
		case vxldollar::error_rpc::bad_multiplier_format:
			return "Bad multiplier";
		case vxldollar::error_rpc::bad_previous:
			return "Bad previous";
		case vxldollar::error_rpc::bad_representative_number:
			return "Bad representative number";
		case vxldollar::error_rpc::bad_source:
			return "Bad source";
		case vxldollar::error_rpc::bad_timeout:
			return "Bad timeout number";
		case vxldollar::error_rpc::bad_work_version:
			return "Bad work version";
		case vxldollar::error_rpc::block_create_balance_mismatch:
			return "Balance mismatch for previous block";
		case vxldollar::error_rpc::block_create_key_required:
			return "Private key or local wallet and account required";
		case vxldollar::error_rpc::block_create_public_key_mismatch:
			return "Incorrect key for given account";
		case vxldollar::error_rpc::block_create_requirements_state:
			return "Previous, representative, final balance and link (source or destination) are required";
		case vxldollar::error_rpc::block_create_requirements_open:
			return "Representative account and source hash required";
		case vxldollar::error_rpc::block_create_requirements_receive:
			return "Previous hash and source hash required";
		case vxldollar::error_rpc::block_create_requirements_change:
			return "Representative account and previous hash required";
		case vxldollar::error_rpc::block_create_requirements_send:
			return "Destination account, previous hash, current balance and amount required";
		case vxldollar::error_rpc::block_root_mismatch:
			return "Root mismatch for block";
		case vxldollar::error_rpc::block_work_enough:
			return "Provided work is already enough for given difficulty";
		case vxldollar::error_rpc::block_work_version_mismatch:
			return "Work version mismatch for block";
		case vxldollar::error_rpc::confirmation_height_not_processing:
			return "There are no blocks currently being processed for adding confirmation height";
		case vxldollar::error_rpc::confirmation_not_found:
			return "Active confirmation not found";
		case vxldollar::error_rpc::difficulty_limit:
			return "Difficulty above config limit or below publish threshold";
		case vxldollar::error_rpc::disabled_bootstrap_lazy:
			return "Lazy bootstrap is disabled";
		case vxldollar::error_rpc::disabled_bootstrap_legacy:
			return "Legacy bootstrap is disabled";
		case vxldollar::error_rpc::invalid_balance:
			return "Invalid balance number";
		case vxldollar::error_rpc::invalid_destinations:
			return "Invalid destinations number";
		case vxldollar::error_rpc::invalid_epoch:
			return "Invalid epoch number";
		case vxldollar::error_rpc::invalid_epoch_signer:
			return "Incorrect epoch signer";
		case vxldollar::error_rpc::invalid_offset:
			return "Invalid offset";
		case vxldollar::error_rpc::invalid_missing_type:
			return "Invalid or missing type argument";
		case vxldollar::error_rpc::invalid_root:
			return "Invalid root hash";
		case vxldollar::error_rpc::invalid_sources:
			return "Invalid sources number";
		case vxldollar::error_rpc::invalid_subtype:
			return "Invalid block subtype";
		case vxldollar::error_rpc::invalid_subtype_balance:
			return "Invalid block balance for given subtype";
		case vxldollar::error_rpc::invalid_subtype_epoch_link:
			return "Invalid epoch link";
		case vxldollar::error_rpc::invalid_subtype_previous:
			return "Invalid previous block for given subtype";
		case vxldollar::error_rpc::invalid_timestamp:
			return "Invalid timestamp";
		case vxldollar::error_rpc::invalid_threads_count:
			return "Invalid threads count";
		case vxldollar::error_rpc::peer_not_found:
			return "Peer not found";
		case vxldollar::error_rpc::pruning_disabled:
			return "Pruning is disabled";
		case vxldollar::error_rpc::requires_port_and_address:
			return "Both port and address required";
		case vxldollar::error_rpc::rpc_control_disabled:
			return "RPC control is disabled";
		case vxldollar::error_rpc::sign_hash_disabled:
			return "Signing by block hash is disabled";
		case vxldollar::error_rpc::source_not_found:
			return "Source not found";
	}

	return "Invalid error code";
}

std::string vxldollar::error_process_messages::message (int ev) const
{
	switch (static_cast<vxldollar::error_process> (ev))
	{
		case vxldollar::error_process::generic:
			return "Unknown error";
		case vxldollar::error_process::bad_signature:
			return "Bad signature";
		case vxldollar::error_process::old:
			return "Old block";
		case vxldollar::error_process::negative_spend:
			return "Negative spend";
		case vxldollar::error_process::fork:
			return "Fork";
		case vxldollar::error_process::unreceivable:
			return "Unreceivable";
		case vxldollar::error_process::gap_previous:
			return "Gap previous block";
		case vxldollar::error_process::gap_source:
			return "Gap source block";
		case vxldollar::error_process::gap_epoch_open_pending:
			return "Gap pending for open epoch block";
		case vxldollar::error_process::opened_burn_account:
			return "Burning account";
		case vxldollar::error_process::balance_mismatch:
			return "Balance and amount delta do not match";
		case vxldollar::error_process::block_position:
			return "This block cannot follow the previous block";
		case vxldollar::error_process::insufficient_work:
			return "Block work is insufficient";
		case vxldollar::error_process::other:
			return "Error processing block";
	}

	return "Invalid error code";
}

std::string vxldollar::error_config_messages::message (int ev) const
{
	switch (static_cast<vxldollar::error_config> (ev))
	{
		case vxldollar::error_config::generic:
			return "Unknown error";
		case vxldollar::error_config::invalid_value:
			return "Invalid configuration value";
		case vxldollar::error_config::missing_value:
			return "Missing value in configuration";
	}

	return "Invalid error code";
}

char const * vxldollar::error_conversion::detail::generic_category::name () const noexcept
{
	return boost::system::generic_category ().name ();
}

std::string vxldollar::error_conversion::detail::generic_category::message (int value) const
{
	return boost::system::generic_category ().message (value);
}

std::error_category const & vxldollar::error_conversion::generic_category ()
{
	static detail::generic_category instance;
	return instance;
}

std::error_code vxldollar::error_conversion::convert (boost::system::error_code const & error)
{
	if (error.category () == boost::system::generic_category ())
	{
		return std::error_code (error.value (),
		vxldollar::error_conversion::generic_category ());
	}

	debug_assert (false);
	return vxldollar::error_common::invalid_type_conversion;
}

vxldollar::error::error (std::error_code code_a)
{
	code = code_a;
}

vxldollar::error::error (boost::system::error_code const & code_a)
{
	code = std::make_error_code (static_cast<std::errc> (code_a.value ()));
}

vxldollar::error::error (std::string message_a)
{
	code = vxldollar::error_common::generic;
	message = std::move (message_a);
}

vxldollar::error::error (std::exception const & exception_a)
{
	code = vxldollar::error_common::exception;
	message = exception_a.what ();
}

vxldollar::error & vxldollar::error::operator= (vxldollar::error const & err_a)
{
	code = err_a.code;
	message = err_a.message;
	return *this;
}

vxldollar::error & vxldollar::error::operator= (vxldollar::error && err_a)
{
	code = err_a.code;
	message = std::move (err_a.message);
	return *this;
}

/** Assign error code */
vxldollar::error & vxldollar::error::operator= (std::error_code const code_a)
{
	code = code_a;
	message.clear ();
	return *this;
}

/** Assign boost error code (as converted to std::error_code) */
vxldollar::error & vxldollar::error::operator= (boost::system::error_code const & code_a)
{
	code = vxldollar::error_conversion::convert (code_a);
	message.clear ();
	return *this;
}

/** Assign boost error code (as converted to std::error_code) */
vxldollar::error & vxldollar::error::operator= (boost::system::errc::errc_t const & code_a)
{
	code = vxldollar::error_conversion::convert (boost::system::errc::make_error_code (code_a));
	message.clear ();
	return *this;
}

/** Set the error to vxldollar::error_common::generic and the error message to \p message_a */
vxldollar::error & vxldollar::error::operator= (std::string message_a)
{
	code = vxldollar::error_common::generic;
	message = std::move (message_a);
	return *this;
}

/** Sets the error to vxldollar::error_common::exception and adopts the exception error message. */
vxldollar::error & vxldollar::error::operator= (std::exception const & exception_a)
{
	code = vxldollar::error_common::exception;
	message = exception_a.what ();
	return *this;
}

/** Return true if this#error_code equals the parameter */
bool vxldollar::error::operator== (std::error_code const code_a) const
{
	return code == code_a;
}

/** Return true if this#error_code equals the parameter */
bool vxldollar::error::operator== (boost::system::error_code const code_a) const
{
	return code.value () == code_a.value ();
}

/** Call the function iff the current error is zero */
vxldollar::error & vxldollar::error::then (std::function<vxldollar::error &()> next)
{
	return code ? *this : next ();
}

/** Implicit error_code conversion */
vxldollar::error::operator std::error_code () const
{
	return code;
}

int vxldollar::error::error_code_as_int () const
{
	return code.value ();
}

/** Implicit bool conversion; true if there's an error */
vxldollar::error::operator bool () const
{
	return code.value () != 0;
}

/** Implicit string conversion; returns the error message or an empty string. */
vxldollar::error::operator std::string () const
{
	return get_message ();
}

/**
	 * Get error message, or an empty string if there's no error. If a custom error message is set,
	 * that will be returned, otherwise the error_code#message() is returned.
	 */
std::string vxldollar::error::get_message () const
{
	std::string res = message;
	if (code && res.empty ())
	{
		res = code.message ();
	}
	return res;
}

/** Set an error message, but only if the error code is already set */
vxldollar::error & vxldollar::error::on_error (std::string message_a)
{
	if (code)
	{
		message = std::move (message_a);
	}
	return *this;
}

/** Set an error message if the current error code matches \p code_a */
vxldollar::error & vxldollar::error::on_error (std::error_code code_a, std::string message_a)
{
	if (code == code_a)
	{
		message = std::move (message_a);
	}
	return *this;
}

/** Set an error message and an error code */
vxldollar::error & vxldollar::error::set (std::string message_a, std::error_code code_a)
{
	message = std::move (message_a);
	code = code_a;
	return *this;
}

/** Set a custom error message. If the error code is not set, it will be set to vxldollar::error_common::generic. */
vxldollar::error & vxldollar::error::set_message (std::string message_a)
{
	if (!code)
	{
		code = vxldollar::error_common::generic;
	}
	message = std::move (message_a);
	return *this;
}

/** Clear an errors */
vxldollar::error & vxldollar::error::clear ()
{
	code.clear ();
	message.clear ();
	return *this;
}

// TODO: theoretically, nothing besides template (partial) specializations should ever be added inside std...
namespace std
{
std::error_code make_error_code (boost::system::errc::errc_t const & e)
{
	return std::error_code (static_cast<int> (e), ::vxldollar::error_conversion::generic_category ());
}
}
