#ifndef G_TESTS_UTILS_STRINGS_TEST_H
#define G_TESTS_UTILS_STRINGS_TEST_H

#include <string>

#include <vector>

#include <gtest/gtest.h>

using namespace std;

class StringsTest : public testing::Test
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

TEST_F(StringsTest, testSplit)
{
	const uint16_t bucket_max = 10;
	char* bucket[10];
	const uint16_t expected_ln = 5;
	const char* expected[5] = {"a", "bb", "ccc", "dddd", "eeeee"};
	size_t bucket_ln;
	string raw = " a    bb ccc  dddd eeeee ";

	bucket_ln = split(&raw[0], " ", bucket, bucket_max);

	ASSERT_EQ(expected_ln, bucket_ln);
	for ( uint16_t i = 0; i < expected_ln; i++ )
		ASSERT_EQ(string(bucket[i]), string(expected[i]));
}

TEST_F(StringsTest, testSplitArgs)
{
	const uint16_t bucket_max = 10;
	char* bucket[10];
	const uint16_t expected_ln = 5;
	const char* expected[5] = {"a", "bb bb bb", "ccc", "dddd", "eeeee"};
	size_t bucket_ln;
	string raw = " a    \"bb bb bb\" ccc  dddd eeeee ";

	bucket_ln = splitArgs(&raw[0], bucket, bucket_max);

	ASSERT_EQ(expected_ln, bucket_ln);
	for ( uint16_t i = 0; i < expected_ln; i++ )
		ASSERT_EQ(string(bucket[i]), string(expected[i]));
}

TEST_F(StringsTest, testSplitArgsCSM)
{
	const uint16_t bucket_max = 10;
	char* bucket[10];
	const uint16_t expected_ln = 5;
	const char* expected[5] = {"a", "bb bb bb", "(cc-c)", "dddd", "eeeee"};
	size_t bucket_ln;
	string raw = " a    (bb bb bb) ((cc-c))  dddd eeeee ";

	bucket_ln = splitArgsCSM(&raw[0], bucket, bucket_max, '(', ')');

	ASSERT_EQ(expected_ln, bucket_ln);
	for ( uint16_t i = 0; i < expected_ln; i++ )
		ASSERT_EQ(string(bucket[i]), string(expected[i]));
}

#endif
