#ifndef G_TESTS_UTILS_CONVERTER_TEST_H
#define G_TESTS_UTILS_CONVERTER_TEST_H

#include <cerrno>

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <string>

#include <vector>

#include <gtest/gtest.h>

#include "../../src/Globals.h"
#include "../../src/utils/Converter.c"

using namespace std;

class ConverterTest : public testing::Test
{
	protected:

	public:

		static void SetUpTestCase()
		{
		}

		static void TearDownTestCase()
		{
		}
};

TEST_F(ConverterTest, testParseUint8)
{
	const vector<const char*> bytes = {
		"01",
		"23",
		"45",
		"67",
		"89",
		"ab",
		"cd",
		"ef",
		"AB",
		"CD",
		"EF",
	};
	uint8_t expteced[] = {
		1,
		35,
		69,
		103,
		137,
		171,
		205,
		239,
		171,
		205,
		239,
	};

	for ( uint32_t i = 0; i < bytes.size(); i++)
	{
		uint8_t r = parseUint8(bytes[i]);
		EXPECT_EQ(r, expteced[i]);
	}
}

TEST_F(ConverterTest, testParseUint8Exceptions)
{
	const vector<const char*> not_hex = {
		"gh",
		"ij",
		"ZZ",
		"GG",
	};
	const vector<const char*> wrong_ln = {
		"g",
		"ifj",
		"",
	};

	for ( uint32_t i = 0; i < not_hex.size(); i++)
	{
		string ex_err_msg = "Error: "+string(not_hex[i])+" is not in hex format!";
		EXPECT_EXIT({parseUint8(not_hex[i]);}, ::testing::ExitedWithCode(0), &ex_err_msg[0]);
	}

	for ( uint32_t i = 0; i < wrong_ln.size(); i++)
	{
		string ex_err_msg = "Error: "+string(wrong_ln[i])+" is not a byte!";
		EXPECT_EXIT({parseUint8(wrong_ln[i]);}, ::testing::ExitedWithCode(0), &ex_err_msg[0]);
	}
}

#endif
