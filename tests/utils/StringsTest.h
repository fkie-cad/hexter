#ifndef G_TESTS_UTILS_STRINGS_TEST_H
#define G_TESTS_UTILS_STRINGS_TEST_H

#include <string>

#include <vector>

#include <gtest/gtest.h>

using namespace std;

class StringsTest : public testing::Test
{
	protected:
		void assertSplit(string& raw, const char** expected, const uint16_t expected_ln);

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
	string raw0 = " a    (bb bb bb) ((cc-c))  dddd eeeee ";
	const uint16_t expected0_ln = 5;
	const char* expected0[5] = {"a", "bb bb bb", "(cc-c)", "dddd", "eeeee"};

	string raw1 = " a    (bb bb bb ((cc-c) ) dddd eeeee ";
	const uint16_t expected1_ln = 2;
	const char* expected1[5] = {"a", "bb bb bb ((cc-c) ) dddd eeeee "};

	assertSplit(raw0, expected0, expected0_ln);
	assertSplit(raw1, expected1, expected1_ln);
}

void StringsTest::assertSplit(string& raw, const char** expected, const uint16_t expected_ln)
{
	const uint16_t bucket_max = 10;
	char* bucket[10];
	size_t bucket_ln;
	bucket_ln = splitArgsCSM(&raw[0], bucket, bucket_max, '(', ')');

	ASSERT_EQ(expected_ln, bucket_ln);
	for ( uint16_t i = 0; i < expected_ln; i++ )
		ASSERT_EQ(string(bucket[i]), string(expected[i]));
}

#endif
