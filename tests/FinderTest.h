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

#include "../src/Globals.h"
#define BLOCKSIZE_LARGE 0x10
#include "../src/Finder.c"

using namespace std;

class FinderTest : public testing::Test
{
	protected:
		static string temp_dir;

		std::random_device rd;
		static mt19937_64* gen;
		static uniform_int_distribution<uint8_t>* dis;

		static string getTempDir(const std::string& prefix)
		{
			string tmp = "/tmp/"+prefix+"XXXXXX";
			char* buf = &tmp[0];
			char* dir = mkdtemp(buf);

			return string(dir);
		}

		vector<uint8_t> createBinary(const string& file_src, size_t f_size)
		{
			ofstream f;
			vector<uint8_t> values;
			values.resize(f_size);

			f.open(file_src, ios::binary | std::ios::out);
			f.clear();

			for ( size_t i = 0; i < f_size; i++ )
			{
				uint8_t value = (*dis)(*gen);
				uint8_t size = sizeof(value);
				f.write(reinterpret_cast<char *>(&(value)), size);
				values[i] = value;
			}

			f.close();

			return values;
		}

		int openFile(const string& command, FILE *&fi) const
		{
			int errsv = errno;
			errno = 0;
			fi = popen(&command[0], "r");

			return errsv;
		}

	public:
		static void SetUpTestCase()
		{
			std::random_device rd;
			gen = new mt19937_64(rd());
			dis = new uniform_int_distribution<uint8_t>(0, UINT8_MAX);

			temp_dir = getTempDir("FinderTest");
		}

		static void TearDownTestCase()
		{
			delete(gen);
			delete(dis);

			rmdir(temp_dir.c_str());
		}
};
mt19937_64* FinderTest::gen = nullptr;
uniform_int_distribution<uint8_t>* FinderTest::dis = nullptr;
string FinderTest::temp_dir;


TEST_F(FinderTest, testFindInFile)
{
	uint64_t binary_size = 64;
	string src = temp_dir+"/testOverwriteInFile.bind";
//	string src = "/tmp/WriterTestSrc.tmp";
	vector<uint8_t> bytes = createBinary(src, binary_size);

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
