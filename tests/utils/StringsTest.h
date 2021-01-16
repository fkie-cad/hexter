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
        void assert_UTF8ToUTF16LE(const unsigned char* v, const vector<uint8_t>& e, size_t on);

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

TEST_F(StringsTest, test_UTF8ToUTF16LE)
{
    const char* v0 = "aBcDz~";
    vector<uint8_t> e0 = {0x61, 0x00, 0x42, 0x00, 0x63, 0x00, 0x44, 0x00, 0x7a, 0x00, 0x7e, 0x00};

    const char* v1 = "öüä";
    vector<uint8_t> e1 = {0xf6, 0x00, 0xfc, 0x00, 0xe4, 0x00};

    const char* v2 = "Привет";
    vector<uint8_t> e2 = {0x1f, 0x04, 0x40, 0x04, 0x38, 0x04, 0x32, 0x04, 0x35, 0x04, 0x42, 0x04};

    const char* v3 = "在干什么";
    vector<uint8_t> e3 = {0x28, 0x57, 0x72, 0x5e, 0xc0, 0x4e, 0x48, 0x4e};

    assert_UTF8ToUTF16LE((unsigned char*)v0, e0, 12); // outb len == expected
    assert_UTF8ToUTF16LE((unsigned char*)v0, e0, 10); // outb len < expected
    assert_UTF8ToUTF16LE((unsigned char*)v0, e0, 14); // outb len > expected
    assert_UTF8ToUTF16LE((unsigned char*)v1, e1, 4);
    assert_UTF8ToUTF16LE((unsigned char*)v2, e2, 14);
    assert_UTF8ToUTF16LE((unsigned char*)v3, e3, 8);
}

void StringsTest::assert_UTF8ToUTF16LE(const unsigned char* v, const vector<uint8_t>& e, size_t on)
{
    unsigned char* outb = (unsigned char*)calloc(on, 1);
    size_t en = e.size();
    if ( on < en )
        en = on;
    size_t vn = strlen((char*)v);

    UTF8ToUTF16LE(outb, &on, v, &vn);

    ASSERT_EQ(en, on);
    for ( size_t i = 0; i < en; i++ )
        ASSERT_EQ(outb[i], e[i]);

    free(outb);
}

#endif
