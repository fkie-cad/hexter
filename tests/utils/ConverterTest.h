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

TEST_F(ConverterTest, testParseUint64)
{
	const vector<const char*> bytes = {
			"0x01",
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
	uint64_t expteced[] = {
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
		uint64_t r;
		int s = parseUint64(bytes[i], &r, 16);
		EXPECT_EQ(s, 0);
		EXPECT_EQ(r, expteced[i]);
	}
}

TEST_F(ConverterTest, testParseUint8)
{
	const vector<const char*> bytes = {
		"1",
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
		uint8_t r;
		int s = parseUint8(bytes[i], &r, 16);
		EXPECT_EQ(s, 0);
		EXPECT_EQ(r, expteced[i]);
	}
}

TEST_F(ConverterTest, testParseUint8Auto)
{
	const vector<const char*> bytes = {
		"1",
		"23",
		"0x45",
		"0x67",
	};
	uint8_t expteced[] = {
		1,
		23,
		69,
		103,
	};

	for ( uint32_t i = 0; i < bytes.size(); i++)
	{
		uint8_t r;
		int s = parseUint8Auto(bytes[i], &r);
		EXPECT_EQ(s, 0);
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

	for (auto i : not_hex)
	{
		uint8_t r;
		int s = parseUint8(i, &r, 16);
		EXPECT_NE(s, 0);
	}

	for (auto i : wrong_ln)
	{
		uint8_t r;
		int s = parseUint8(i, &r, 16);
		EXPECT_NE(s, 0);
	}
}

#endif
