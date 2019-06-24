#ifndef G_TESTS_FINDER_TEST_H
#define G_TESTS_FINDER_TEST_H

#include <cerrno>

#include <cstdio>
#include <cstdlib>
#include <cstdint>

#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

#include <gtest/gtest.h>
#include <deque>

#include "misc/Misc.h"
#include "../src/Globals.h"
#define BLOCKSIZE_LARGE 0x10
#include "../src/Finder.c"

using namespace std;

class FinderTest : public testing::Test
{
	protected:
		static string temp_dir;

		static Misc misc;

		static string getTempDir(const std::string& prefix)
		{
			string tmp = "/tmp/"+prefix+"XXXXXX";
			char* buf = &tmp[0];
			char* dir = mkdtemp(buf);

			return string(dir);
		}

	public:
		static void SetUpTestCase()
		{
			temp_dir = getTempDir("FinderTest");
		}

		static void TearDownTestCase()
		{
			rmdir(temp_dir.c_str());
		}
};
Misc FinderTest::misc;
string FinderTest::temp_dir;


TEST_F(FinderTest, testFindInFile)
{
	uint64_t binary_size = 64;
	string src = temp_dir+"/testFindInFile.rand";
//	string src = "/tmp/WriterTestSrc.tmp";
	vector<uint8_t> bytes = misc.createBinary(src, binary_size);

	unsigned char* payload = &bytes.data()[10];
	uint32_t payload_ln = 8;
	snprintf(file_name, PATH_MAX, "%s", &src[0]);
	start = 0;
	file_size = getSize(file_name);

	uint64_t idx = find(payload, payload_ln, 0);

	EXPECT_LT(idx, UINT64_MAX);

	if ( idx < UINT64_MAX )
	{
		ifstream check_fs(src);
		check_fs.seekg(idx);

		for ( int i = 0; i < payload_ln; i++ )
		{
			unsigned char cs;
			check_fs.read(reinterpret_cast<char*>(&(cs)), 1);
			cout << setw(2) << setfill('0') << i << hex << " : " << +cs << " = " << +payload[i] << dec << endl;

			EXPECT_EQ(cs, payload[i]);
		}
	}

	remove(src.c_str());
}

#endif
