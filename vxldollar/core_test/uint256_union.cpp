#include <vxldollar/crypto_lib/random_pool.hpp>
#include <vxldollar/secure/common.hpp>
#include <vxldollar/test_common/testutil.hpp>

#include <gtest/gtest.h>

#include <thread>

namespace
{
template <typename Union, typename Bound>
void assert_union_types ();

template <typename Union, typename Bound>
void test_union_operator_less_than ();

template <typename Num>
void check_operator_less_than (Num lhs, Num rhs);

template <typename Union, typename Bound>
void test_union_operator_greater_than ();

template <typename Num>
void check_operator_greater_than (Num lhs, Num rhs);
}

TEST (uint128_union, decode_dec)
{
	vxldollar::uint128_union value;
	std::string text ("16");
	ASSERT_FALSE (value.decode_dec (text));
	ASSERT_EQ (16, value.bytes[15]);
}

TEST (uint128_union, decode_dec_negative)
{
	vxldollar::uint128_union value;
	std::string text ("-1");
	auto error (value.decode_dec (text));
	ASSERT_TRUE (error);
}

TEST (uint128_union, decode_dec_zero)
{
	vxldollar::uint128_union value;
	std::string text ("0");
	ASSERT_FALSE (value.decode_dec (text));
	ASSERT_TRUE (value.is_zero ());
}

TEST (uint128_union, decode_dec_leading_zero)
{
	vxldollar::uint128_union value;
	std::string text ("010");
	auto error (value.decode_dec (text));
	ASSERT_TRUE (error);
}

TEST (uint128_union, decode_dec_overflow)
{
	vxldollar::uint128_union value;
	std::string text ("340282366920938463463374607431768211456");
	auto error (value.decode_dec (text));
	ASSERT_TRUE (error);
}

TEST (uint128_union, operator_less_than)
{
	test_union_operator_less_than<vxldollar::uint128_union, vxldollar::uint128_t> ();
}

TEST (uint128_union, operator_greater_than)
{
	test_union_operator_greater_than<vxldollar::uint128_union, vxldollar::uint128_t> ();
}

struct test_punct : std::moneypunct<char>
{
	pattern do_pos_format () const
	{
		return { { value, none, none, none } };
	}
	int do_frac_digits () const
	{
		return 0;
	}
	char_type do_decimal_point () const
	{
		return '+';
	}
	char_type do_thousands_sep () const
	{
		return '-';
	}
	string_type do_grouping () const
	{
		return "\3\4";
	}
};

TEST (uint128_union, balance_format)
{
	ASSERT_EQ ("0", vxldollar::amount (vxldollar::uint128_t ("0")).format_balance (vxldollar::Mxrb_ratio, 0, false));
	ASSERT_EQ ("0", vxldollar::amount (vxldollar::uint128_t ("0")).format_balance (vxldollar::Mxrb_ratio, 2, true));
	ASSERT_EQ ("340,282,366", vxldollar::amount (vxldollar::uint128_t ("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF")).format_balance (vxldollar::Mxrb_ratio, 0, true));
	ASSERT_EQ ("340,282,366.920938463463374607431768211455", vxldollar::amount (vxldollar::uint128_t ("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF")).format_balance (vxldollar::Mxrb_ratio, 64, true));
	ASSERT_EQ ("340,282,366,920,938,463,463,374,607,431,768,211,455", vxldollar::amount (vxldollar::uint128_t ("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF")).format_balance (1, 4, true));
	ASSERT_EQ ("340,282,366", vxldollar::amount (vxldollar::uint128_t ("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFE")).format_balance (vxldollar::Mxrb_ratio, 0, true));
	ASSERT_EQ ("340,282,366.920938463463374607431768211454", vxldollar::amount (vxldollar::uint128_t ("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFE")).format_balance (vxldollar::Mxrb_ratio, 64, true));
	ASSERT_EQ ("340282366920938463463374607431768211454", vxldollar::amount (vxldollar::uint128_t ("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFE")).format_balance (1, 4, false));
	ASSERT_EQ ("170,141,183", vxldollar::amount (vxldollar::uint128_t ("0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFE")).format_balance (vxldollar::Mxrb_ratio, 0, true));
	ASSERT_EQ ("170,141,183.460469231731687303715884105726", vxldollar::amount (vxldollar::uint128_t ("0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFE")).format_balance (vxldollar::Mxrb_ratio, 64, true));
	ASSERT_EQ ("170141183460469231731687303715884105726", vxldollar::amount (vxldollar::uint128_t ("0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFE")).format_balance (1, 4, false));
	ASSERT_EQ ("1", vxldollar::amount (vxldollar::uint128_t ("1000000000000000000000000000000")).format_balance (vxldollar::Mxrb_ratio, 2, true));
	ASSERT_EQ ("1.2", vxldollar::amount (vxldollar::uint128_t ("1200000000000000000000000000000")).format_balance (vxldollar::Mxrb_ratio, 2, true));
	ASSERT_EQ ("1.23", vxldollar::amount (vxldollar::uint128_t ("1230000000000000000000000000000")).format_balance (vxldollar::Mxrb_ratio, 2, true));
	ASSERT_EQ ("1.2", vxldollar::amount (vxldollar::uint128_t ("1230000000000000000000000000000")).format_balance (vxldollar::Mxrb_ratio, 1, true));
	ASSERT_EQ ("1", vxldollar::amount (vxldollar::uint128_t ("1230000000000000000000000000000")).format_balance (vxldollar::Mxrb_ratio, 0, true));
	ASSERT_EQ ("< 0.01", vxldollar::amount (vxldollar::xrb_ratio * 10).format_balance (vxldollar::Mxrb_ratio, 2, true));
	ASSERT_EQ ("< 0.1", vxldollar::amount (vxldollar::xrb_ratio * 10).format_balance (vxldollar::Mxrb_ratio, 1, true));
	ASSERT_EQ ("< 1", vxldollar::amount (vxldollar::xrb_ratio * 10).format_balance (vxldollar::Mxrb_ratio, 0, true));
	ASSERT_EQ ("< 0.01", vxldollar::amount (vxldollar::xrb_ratio * 9999).format_balance (vxldollar::Mxrb_ratio, 2, true));
	ASSERT_EQ ("0.01", vxldollar::amount (vxldollar::xrb_ratio * 10000).format_balance (vxldollar::Mxrb_ratio, 2, true));
	ASSERT_EQ ("123456789", vxldollar::amount (vxldollar::Mxrb_ratio * 123456789).format_balance (vxldollar::Mxrb_ratio, 2, false));
	ASSERT_EQ ("123,456,789", vxldollar::amount (vxldollar::Mxrb_ratio * 123456789).format_balance (vxldollar::Mxrb_ratio, 2, true));
	ASSERT_EQ ("123,456,789.12", vxldollar::amount (vxldollar::Mxrb_ratio * 123456789 + vxldollar::kxrb_ratio * 123).format_balance (vxldollar::Mxrb_ratio, 2, true));
	ASSERT_EQ ("12-3456-789+123", vxldollar::amount (vxldollar::Mxrb_ratio * 123456789 + vxldollar::kxrb_ratio * 123).format_balance (vxldollar::Mxrb_ratio, 4, true, std::locale (std::cout.getloc (), new test_punct)));
}

TEST (uint128_union, decode_decimal)
{
	vxldollar::amount amount;
	ASSERT_FALSE (amount.decode_dec ("340282366920938463463374607431768211455", vxldollar::raw_ratio));
	ASSERT_EQ (std::numeric_limits<vxldollar::uint128_t>::max (), amount.number ());
	ASSERT_TRUE (amount.decode_dec ("340282366920938463463374607431768211456", vxldollar::raw_ratio));
	ASSERT_TRUE (amount.decode_dec ("340282366920938463463374607431768211455.1", vxldollar::raw_ratio));
	ASSERT_TRUE (amount.decode_dec ("0.1", vxldollar::raw_ratio));
	ASSERT_FALSE (amount.decode_dec ("1", vxldollar::raw_ratio));
	ASSERT_EQ (1, amount.number ());
	ASSERT_FALSE (amount.decode_dec ("340282366.920938463463374607431768211454", vxldollar::Mxrb_ratio));
	ASSERT_EQ (std::numeric_limits<vxldollar::uint128_t>::max () - 1, amount.number ());
	ASSERT_TRUE (amount.decode_dec ("340282366.920938463463374607431768211456", vxldollar::Mxrb_ratio));
	ASSERT_TRUE (amount.decode_dec ("340282367", vxldollar::Mxrb_ratio));
	ASSERT_FALSE (amount.decode_dec ("0.000000000000000000000001", vxldollar::Mxrb_ratio));
	ASSERT_EQ (1000000, amount.number ());
	ASSERT_FALSE (amount.decode_dec ("0.000000000000000000000000000001", vxldollar::Mxrb_ratio));
	ASSERT_EQ (1, amount.number ());
	ASSERT_TRUE (amount.decode_dec ("0.0000000000000000000000000000001", vxldollar::Mxrb_ratio));
	ASSERT_TRUE (amount.decode_dec (".1", vxldollar::Mxrb_ratio));
	ASSERT_TRUE (amount.decode_dec ("0.", vxldollar::Mxrb_ratio));
	ASSERT_FALSE (amount.decode_dec ("9.999999999999999999999999999999", vxldollar::Mxrb_ratio));
	ASSERT_EQ (vxldollar::uint128_t ("9999999999999999999999999999999"), amount.number ());
	ASSERT_FALSE (amount.decode_dec ("170141183460469.231731687303715884105727", vxldollar::xrb_ratio));
	ASSERT_EQ (vxldollar::uint128_t ("170141183460469231731687303715884105727"), amount.number ());
	ASSERT_FALSE (amount.decode_dec ("2.000000000000000000000002", vxldollar::xrb_ratio));
	ASSERT_EQ (2 * vxldollar::xrb_ratio + 2, amount.number ());
	ASSERT_FALSE (amount.decode_dec ("2", vxldollar::xrb_ratio));
	ASSERT_EQ (2 * vxldollar::xrb_ratio, amount.number ());
	ASSERT_FALSE (amount.decode_dec ("1230", vxldollar::Gxrb_ratio));
	ASSERT_EQ (1230 * vxldollar::Gxrb_ratio, amount.number ());
}

TEST (unions, identity)
{
	ASSERT_EQ (1, vxldollar::uint128_union (1).number ().convert_to<uint8_t> ());
	ASSERT_EQ (1, vxldollar::uint256_union (1).number ().convert_to<uint8_t> ());
	ASSERT_EQ (1, vxldollar::uint512_union (1).number ().convert_to<uint8_t> ());
}

TEST (uint256_union, key_encryption)
{
	vxldollar::keypair key1;
	vxldollar::raw_key secret_key;
	secret_key.clear ();
	vxldollar::uint256_union encrypted;
	encrypted.encrypt (key1.prv, secret_key, key1.pub.owords[0]);
	vxldollar::raw_key key4;
	key4.decrypt (encrypted, secret_key, key1.pub.owords[0]);
	ASSERT_EQ (key1.prv, key4);
	auto pub (vxldollar::pub_key (key4));
	ASSERT_EQ (key1.pub, pub);
}

TEST (uint256_union, encryption)
{
	vxldollar::raw_key key;
	key.clear ();
	vxldollar::raw_key number1;
	number1 = 1;
	vxldollar::uint256_union encrypted1;
	encrypted1.encrypt (number1, key, key.owords[0]);
	vxldollar::uint256_union encrypted2;
	encrypted2.encrypt (number1, key, key.owords[0]);
	ASSERT_EQ (encrypted1, encrypted2);
	vxldollar::raw_key number2;
	number2.decrypt (encrypted1, key, key.owords[0]);
	ASSERT_EQ (number1, number2);
}

TEST (uint256_union, decode_empty)
{
	std::string text;
	vxldollar::uint256_union val;
	ASSERT_TRUE (val.decode_hex (text));
}

TEST (uint256_union, parse_zero)
{
	vxldollar::uint256_union input (vxldollar::uint256_t (0));
	std::string text;
	input.encode_hex (text);
	vxldollar::uint256_union output;
	auto error (output.decode_hex (text));
	ASSERT_FALSE (error);
	ASSERT_EQ (input, output);
	ASSERT_TRUE (output.number ().is_zero ());
}

TEST (uint256_union, parse_zero_short)
{
	std::string text ("0");
	vxldollar::uint256_union output;
	auto error (output.decode_hex (text));
	ASSERT_FALSE (error);
	ASSERT_TRUE (output.number ().is_zero ());
}

TEST (uint256_union, parse_one)
{
	vxldollar::uint256_union input (vxldollar::uint256_t (1));
	std::string text;
	input.encode_hex (text);
	vxldollar::uint256_union output;
	auto error (output.decode_hex (text));
	ASSERT_FALSE (error);
	ASSERT_EQ (input, output);
	ASSERT_EQ (1, output.number ());
}

TEST (uint256_union, parse_error_symbol)
{
	vxldollar::uint256_union input (vxldollar::uint256_t (1000));
	std::string text;
	input.encode_hex (text);
	text[5] = '!';
	vxldollar::uint256_union output;
	auto error (output.decode_hex (text));
	ASSERT_TRUE (error);
}

TEST (uint256_union, max_hex)
{
	vxldollar::uint256_union input (std::numeric_limits<vxldollar::uint256_t>::max ());
	std::string text;
	input.encode_hex (text);
	vxldollar::uint256_union output;
	auto error (output.decode_hex (text));
	ASSERT_FALSE (error);
	ASSERT_EQ (input, output);
	ASSERT_EQ (vxldollar::uint256_t ("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"), output.number ());
}

TEST (uint256_union, decode_dec)
{
	vxldollar::uint256_union value;
	std::string text ("16");
	ASSERT_FALSE (value.decode_dec (text));
	ASSERT_EQ (16, value.bytes[31]);
}

TEST (uint256_union, max_dec)
{
	vxldollar::uint256_union input (std::numeric_limits<vxldollar::uint256_t>::max ());
	std::string text;
	input.encode_dec (text);
	vxldollar::uint256_union output;
	auto error (output.decode_dec (text));
	ASSERT_FALSE (error);
	ASSERT_EQ (input, output);
	ASSERT_EQ (vxldollar::uint256_t ("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"), output.number ());
}

TEST (uint256_union, decode_dec_negative)
{
	vxldollar::uint256_union value;
	std::string text ("-1");
	auto error (value.decode_dec (text));
	ASSERT_TRUE (error);
}

TEST (uint256_union, decode_dec_zero)
{
	vxldollar::uint256_union value;
	std::string text ("0");
	ASSERT_FALSE (value.decode_dec (text));
	ASSERT_TRUE (value.is_zero ());
}

TEST (uint256_union, decode_dec_leading_zero)
{
	vxldollar::uint256_union value;
	std::string text ("010");
	auto error (value.decode_dec (text));
	ASSERT_TRUE (error);
}

TEST (uint256_union, parse_error_overflow)
{
	vxldollar::uint256_union input (std::numeric_limits<vxldollar::uint256_t>::max ());
	std::string text;
	input.encode_hex (text);
	text.push_back (0);
	vxldollar::uint256_union output;
	auto error (output.decode_hex (text));
	ASSERT_TRUE (error);
}

TEST (uint256_union, big_endian_union_constructor)
{
	vxldollar::uint256_t value1 (1);
	vxldollar::uint256_union bytes1 (value1);
	ASSERT_EQ (1, bytes1.bytes[31]);
	vxldollar::uint512_t value2 (1);
	vxldollar::uint512_union bytes2 (value2);
	ASSERT_EQ (1, bytes2.bytes[63]);
}

TEST (uint256_union, big_endian_union_function)
{
	vxldollar::uint256_union bytes1 ("FEDCBA9876543210FEDCBA9876543210FEDCBA9876543210FEDCBA9876543210");
	ASSERT_EQ (0xfe, bytes1.bytes[0x00]);
	ASSERT_EQ (0xdc, bytes1.bytes[0x01]);
	ASSERT_EQ (0xba, bytes1.bytes[0x02]);
	ASSERT_EQ (0x98, bytes1.bytes[0x03]);
	ASSERT_EQ (0x76, bytes1.bytes[0x04]);
	ASSERT_EQ (0x54, bytes1.bytes[0x05]);
	ASSERT_EQ (0x32, bytes1.bytes[0x06]);
	ASSERT_EQ (0x10, bytes1.bytes[0x07]);
	ASSERT_EQ (0xfe, bytes1.bytes[0x08]);
	ASSERT_EQ (0xdc, bytes1.bytes[0x09]);
	ASSERT_EQ (0xba, bytes1.bytes[0x0a]);
	ASSERT_EQ (0x98, bytes1.bytes[0x0b]);
	ASSERT_EQ (0x76, bytes1.bytes[0x0c]);
	ASSERT_EQ (0x54, bytes1.bytes[0x0d]);
	ASSERT_EQ (0x32, bytes1.bytes[0x0e]);
	ASSERT_EQ (0x10, bytes1.bytes[0x0f]);
	ASSERT_EQ (0xfe, bytes1.bytes[0x10]);
	ASSERT_EQ (0xdc, bytes1.bytes[0x11]);
	ASSERT_EQ (0xba, bytes1.bytes[0x12]);
	ASSERT_EQ (0x98, bytes1.bytes[0x13]);
	ASSERT_EQ (0x76, bytes1.bytes[0x14]);
	ASSERT_EQ (0x54, bytes1.bytes[0x15]);
	ASSERT_EQ (0x32, bytes1.bytes[0x16]);
	ASSERT_EQ (0x10, bytes1.bytes[0x17]);
	ASSERT_EQ (0xfe, bytes1.bytes[0x18]);
	ASSERT_EQ (0xdc, bytes1.bytes[0x19]);
	ASSERT_EQ (0xba, bytes1.bytes[0x1a]);
	ASSERT_EQ (0x98, bytes1.bytes[0x1b]);
	ASSERT_EQ (0x76, bytes1.bytes[0x1c]);
	ASSERT_EQ (0x54, bytes1.bytes[0x1d]);
	ASSERT_EQ (0x32, bytes1.bytes[0x1e]);
	ASSERT_EQ (0x10, bytes1.bytes[0x1f]);
	ASSERT_EQ ("FEDCBA9876543210FEDCBA9876543210FEDCBA9876543210FEDCBA9876543210", bytes1.to_string ());
	ASSERT_EQ (vxldollar::uint256_t ("0xFEDCBA9876543210FEDCBA9876543210FEDCBA9876543210FEDCBA9876543210"), bytes1.number ());
	vxldollar::uint512_union bytes2;
	bytes2.clear ();
	bytes2.bytes[63] = 1;
	ASSERT_EQ (vxldollar::uint512_t (1), bytes2.number ());
}

TEST (uint256_union, decode_vxldollar_variant)
{
	vxldollar::account key;
	ASSERT_FALSE (key.decode_account ("vxld_1111111111111111111111111111111111111111111111111111hifc8npp"));
	ASSERT_FALSE (key.decode_account ("vxld_1111111111111111111111111111111111111111111111111111hifc8npp"));
}

TEST (uint256_union, account_transcode)
{
	vxldollar::account value;
	auto text (vxldollar::dev::genesis_key.pub.to_account ());
	ASSERT_FALSE (value.decode_account (text));
	ASSERT_EQ (vxldollar::dev::genesis_key.pub, value);

	/*
	 * Handle different offsets for the underscore separator
	 * for "xrb_" prefixed and "vxldollar_" prefixed accounts
	 */
	 // NUMEROS CARACTERES DA HASH 4 letras DE vxld
	unsigned offset = 4;
	ASSERT_EQ ('_', text[offset]);
	text[offset] = '-';
	vxldollar::account value2;
	ASSERT_FALSE (value2.decode_account (text));
	ASSERT_EQ (value, value2);
}

TEST (uint256_union, account_encode_lex)
{
	vxldollar::account min ("0000000000000000000000000000000000000000000000000000000000000000");
	vxldollar::account max ("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
	auto min_text (min.to_account ());
	auto max_text (max.to_account ());

	/*
	 * Handle different lengths for "xrb_" prefixed and "vxldollar_" prefixed accounts
	 */
	 // NUMEROS CARACTERES DA HASH 60 + 5 (4 letras + underscore) de vxld_
	unsigned length = 65;
	ASSERT_EQ (length, min_text.size ());
	ASSERT_EQ (length, max_text.size ());

	auto previous (min_text);
	for (auto i (1); i != 1000; ++i)
	{
		vxldollar::account number (min.number () + i);
		auto text (number.to_account ());
		vxldollar::account output;
		output.decode_account (text);
		ASSERT_EQ (number, output);
		ASSERT_GT (text, previous);
		previous = text;
	}
	for (auto i (1); i != 1000; ++i)
	{
		vxldollar::keypair key;
		auto text (key.pub.to_account ());
		vxldollar::account output;
		output.decode_account (text);
		ASSERT_EQ (key.pub, output);
	}
}

TEST (uint256_union, bounds)
{
	vxldollar::account key;
	std::string bad1 (65, '\x000');//
	bad1[0] = 'v';
	bad1[1] = 'x';
	bad1[2] = 'l';
	bad1[3] = 'd';
	bad1[4] = '-';
	ASSERT_TRUE (key.decode_account (bad1));
	std::string bad2 (65, '\x0ff');//
	bad2[0] = 'v';
	bad2[1] = 'x';
	bad2[2] = 'l';
	bad2[3] = 'd';
	bad2[4] = '-';
	ASSERT_TRUE (key.decode_account (bad2));
}

TEST (uint256_union, operator_less_than)
{
	test_union_operator_less_than<vxldollar::uint256_union, vxldollar::uint256_t> ();
}

TEST (uint64_t, parse)
{
	uint64_t value0 (1);
	ASSERT_FALSE (vxldollar::from_string_hex ("0", value0));
	ASSERT_EQ (0, value0);
	uint64_t value1 (1);
	ASSERT_FALSE (vxldollar::from_string_hex ("ffffffffffffffff", value1));
	ASSERT_EQ (0xffffffffffffffffULL, value1);
	uint64_t value2 (1);
	ASSERT_TRUE (vxldollar::from_string_hex ("g", value2));
	uint64_t value3 (1);
	ASSERT_TRUE (vxldollar::from_string_hex ("ffffffffffffffff0", value3));
	uint64_t value4 (1);
	ASSERT_TRUE (vxldollar::from_string_hex ("", value4));
}

TEST (uint256_union, hash)
{
	ASSERT_EQ (4, vxldollar::uint256_union{}.qwords.size ());
	std::hash<vxldollar::uint256_union> h{};
	for (size_t i (0), n (vxldollar::uint256_union{}.bytes.size ()); i < n; ++i)
	{
		vxldollar::uint256_union x1{ 0 };
		vxldollar::uint256_union x2{ 0 };
		x2.bytes[i] = 1;
		ASSERT_NE (h (x1), h (x2));
	}
}

TEST (uint512_union, hash)
{
	ASSERT_EQ (2, vxldollar::uint512_union{}.uint256s.size ());
	std::hash<vxldollar::uint512_union> h{};
	for (size_t i (0), n (vxldollar::uint512_union{}.bytes.size ()); i < n; ++i)
	{
		vxldollar::uint512_union x1{ 0 };
		vxldollar::uint512_union x2{ 0 };
		x2.bytes[i] = 1;
		ASSERT_NE (h (x1), h (x2));
	}
	for (auto part (0); part < vxldollar::uint512_union{}.uint256s.size (); ++part)
	{
		for (size_t i (0), n (vxldollar::uint512_union{}.uint256s[part].bytes.size ()); i < n; ++i)
		{
			vxldollar::uint512_union x1{ 0 };
			vxldollar::uint512_union x2{ 0 };
			x2.uint256s[part].bytes[i] = 1;
			ASSERT_NE (h (x1), h (x2));
		}
	}
}

namespace
{
template <typename Union, typename Bound>
void assert_union_types ()
{
	static_assert ((std::is_same<Union, vxldollar::uint128_union>::value && std::is_same<Bound, vxldollar::uint128_t>::value) || (std::is_same<Union, vxldollar::uint256_union>::value && std::is_same<Bound, vxldollar::uint256_t>::value) || (std::is_same<Union, vxldollar::uint512_union>::value && std::is_same<Bound, vxldollar::uint512_t>::value),
	"Union type needs to be consistent with the lower/upper Bound type");
}

template <typename Union, typename Bound>
void test_union_operator_less_than ()
{
	assert_union_types<Union, Bound> ();

	// Small
	check_operator_less_than (Union (123), Union (124));
	check_operator_less_than (Union (124), Union (125));

	// Medium
	check_operator_less_than (Union (std::numeric_limits<uint16_t>::max () - 1), Union (std::numeric_limits<uint16_t>::max () + 1));
	check_operator_less_than (Union (std::numeric_limits<uint32_t>::max () - 12345678), Union (std::numeric_limits<uint32_t>::max () - 123456));

	// Large
	check_operator_less_than (Union (std::numeric_limits<uint64_t>::max () - 555555555555), Union (std::numeric_limits<uint64_t>::max () - 1));

	// Boundary values
	check_operator_less_than (Union (std::numeric_limits<Bound>::min ()), Union (std::numeric_limits<Bound>::max ()));
}

template <typename Num>
void check_operator_less_than (Num lhs, Num rhs)
{
	ASSERT_TRUE (lhs < rhs);
	ASSERT_FALSE (rhs < lhs);
	ASSERT_FALSE (lhs < lhs);
	ASSERT_FALSE (rhs < rhs);
}

template <typename Union, typename Bound>
void test_union_operator_greater_than ()
{
	assert_union_types<Union, Bound> ();

	// Small
	check_operator_greater_than (Union (124), Union (123));
	check_operator_greater_than (Union (125), Union (124));

	// Medium
	check_operator_greater_than (Union (std::numeric_limits<uint16_t>::max () + 1), Union (std::numeric_limits<uint16_t>::max () - 1));
	check_operator_greater_than (Union (std::numeric_limits<uint32_t>::max () - 123456), Union (std::numeric_limits<uint32_t>::max () - 12345678));

	// Large
	check_operator_greater_than (Union (std::numeric_limits<uint64_t>::max () - 1), Union (std::numeric_limits<uint64_t>::max () - 555555555555));

	// Boundary values
	check_operator_greater_than (Union (std::numeric_limits<Bound>::max ()), Union (std::numeric_limits<Bound>::min ()));
}

template <typename Num>
void check_operator_greater_than (Num lhs, Num rhs)
{
	ASSERT_TRUE (lhs > rhs);
	ASSERT_FALSE (rhs > lhs);
	ASSERT_FALSE (lhs > lhs);
	ASSERT_FALSE (rhs > rhs);
}
}

TEST (random_pool, multithreading)
{
	std::vector<std::thread> threads;
	for (auto i = 0; i < 100; ++i)
	{
		threads.emplace_back ([] () {
			vxldollar::uint256_union number;
			vxldollar::random_pool::generate_block (number.bytes.data (), number.bytes.size ());
		});
	}
	for (auto & i : threads)
	{
		i.join ();
	}
}
