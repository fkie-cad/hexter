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
//#include "../src/Finder.c"

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

		void findNeedle(unsigned char* needle, uint32_t needle_ln, uint64_t expected_idx, vector<uint8_t>& bytes, const string& src, const uint64_t offset);

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

	snprintf(file_path, PATH_MAX, "%s", &src[0]);
	uint64_t start = 0;
	file_size = getSize(file_path);

	vector<uint8_t> bytes = misc.createBinary(src, binary_size);
	uint16_t needle_idx = 10;

	unsigned char* needle = &bytes[needle_idx];
	uint32_t needle_ln = 8;

	findNeedle(needle, needle_ln, needle_idx, bytes, src, start);

	remove(src.c_str());
}

TEST_F(FinderTest, testFindWithFailure)
{
	uint64_t binary_size = 64;
	string src = temp_dir+"/testFindInFile.rand";

	snprintf(file_path, PATH_MAX, "%s", &src[0]);
	uint64_t start = 0;
	file_size = getSize(file_path);

	vector<uint8_t> bytes = { 1,1,3,4,5,6,7,8,
						   	  1,2,2,4,5,6,7,8,
						   	  1,2,3,3,5,6,7,8,
						   	  1,2,3,4,4,6,7,8,
						   	  1,2,3,4,5,5,7,8,
						   	  1,2,3,4,5,6,6,8,
						   	  1,2,3,4,5,6,7,7,
						   	  1,2,3,4,5,6,7,8 };
	misc.createBinary(src, bytes);
	uint16_t needle_idx = 56;

	unsigned char* needle = &bytes[needle_idx];
	uint32_t needle_ln = 8;

	findNeedle(needle, needle_ln, needle_idx, bytes, src, start);

	remove(src.c_str());
}

TEST_F(FinderTest, testNotFound)
{
	uint64_t binary_size = 64;
	string src = temp_dir+"/testFindInFile.rand";

	snprintf(file_path, PATH_MAX, "%s", &src[0]);
	uint64_t start = 0;
	file_size = getSize(file_path);

	vector<uint8_t> bytes = misc.createBinary(src, binary_size);

	unsigned char needle[] = {0,0,0,0,0,0,0,0};
	uint32_t needle_ln = 8;
	uint64_t expected_idx = UINT64_MAX;

	findNeedle(needle, needle_ln, expected_idx, bytes, src, start);

	remove(src.c_str());
}

void FinderTest::findNeedle(unsigned char* needle, uint32_t needle_ln, uint64_t expected_idx, vector<uint8_t>& bytes, const string& src, const uint64_t offset)
{
	Finder_initFailure(needle, needle_ln);
	file_size = getSize(file_path);

	uint64_t found = find(&src[0], needle, needle_ln, offset, file_size);

	EXPECT_EQ(found, expected_idx);

	if ( found < UINT64_MAX )
	{
		ifstream check_fs(src);
		check_fs.seekg(found);

		for ( uint32_t i = 0; i < needle_ln; i++ )
		{
			unsigned char cs;
			check_fs.read(reinterpret_cast<char*>(&(cs)), 1);
			cout << setw(2) << setfill('0') << found+i << " : (h) " << hex << +cs << " = " << +needle[i] << dec << " (n)"<<endl;

			EXPECT_EQ(cs, needle[i]);
		}
	}

	Finder_cleanUp();
}

#endif
