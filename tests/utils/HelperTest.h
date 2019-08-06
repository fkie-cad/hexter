#ifndef G_TESTS_UTILS_HELPER_TEST_H
#define G_TESTS_UTILS_HELPER_TEST_H

#include <time.h>
#include <cerrno>

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <string>

#include <vector>

#include <gtest/gtest.h>

#include "../../src/Globals.h"
//#include "../../src/utils/Helper.c"

using namespace std;

class HelperTest : public testing::Test
{
	protected:
		using PNP = pair<char*, const char*>;

		void testGetFileNameP(vector<PNP> pathes);
		void testGetFileNameL(vector<PNP> pathes);
		void testGetFileNameA(vector<PNP> pathes);

	public:

		static void SetUpTestCase()
		{
		}

		static void TearDownTestCase()
		{
		}
};

TEST_F(HelperTest, testCountHexWidth64)
{
	vector<pair<uint64_t,uint8_t>> values = {
		{0x1234567890abcdef,16},
		{0x0234567890abcdef,16},
		{0x0034567890abcdef,14},
		{0x0004567890abcdef,14},
		{0x0000567890abcdef,12},
		{0x0000067890abcdef,12},
		{0x0000007890abcdef,10},
		{0x0000000890abcdef,10},
		{0x0000000090abcdef,8},
		{0x000000000fabcdef,8},
		{0x0000000000abcdef,6},
		{0x00000000000bcdef,6},
		{0x000000000000cdef,4},
		{0x0000000000000def,4},
		{0x00000000000000ef,2},
		{0x000000000000000f,2},
	};

	for ( pair<uint64_t,uint8_t> p : values )
	{
		uint8_t width = countHexWidth64(p.first);
		EXPECT_EQ(width, p.second);
	}
}

TEST_F(HelperTest, testGetFileName)
{
	vector<PNP> pathes = {
		{"a/file/path", "path"},
		{"an/other/file/path.type", "path.type"},
		{"path.type", "path.type"},
		{"/path.type", "path.type"},
		{"", NULL},
	};

//	testGetFileNameP(pathes);
//	testGetFileNameL(pathes);
	testGetFileNameA(pathes);
}

void HelperTest::testGetFileNameP(vector<PNP> pathes)
{
	for ( PNP& p : pathes )
	{
		cout << p.first << " : "<<p.second << endl;
		char* file_name = getFileNameP(p.first);
		cout << " = "<<file_name<<endl;
		EXPECT_STREQ(file_name, p.second);
		free(file_name);
	}
}

void HelperTest::testGetFileNameL(vector<PNP> pathes)
{
	for ( PNP& p : pathes )
	{
		cout << p.first << " : "<<p.second << endl;
		char* file_name = NULL;
		getFileNameL(p.first, &file_name);
		cout << " = "<<file_name<<endl;
		EXPECT_STREQ(file_name, p.second);
	}
}

void HelperTest::testGetFileNameA(vector<PNP> pathes)
{
	for ( PNP& p : pathes )
	{
		cout << p.first << " : "<<p.second << endl;
		char file_name[PATH_MAX] = {0};
		getFileName(p.first, file_name);
		cout << " = "<<file_name<<endl;
		if ( p.second == NULL )
			EXPECT_STREQ(file_name, "");
		else
			EXPECT_STREQ(file_name, p.second);
	}
}

TEST_F(HelperTest, testSplit)
{
	cerr << "HelperTest::testSplit()"<<endl;
}

TEST_F(HelperTest, testSplitArgs)
{
	cerr << "HelperTest::testSplitArgs()"<<endl;
}

#endif
