#ifndef G_TESTS_PAYLOADER_TEST_H
#define G_TESTS_PAYLOADER_TEST_H

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

#include "../src/utils/common_fileio.c"
#include "../src/utils/Helper.c"
#include "../src/Globals.h"
#include "../src/Payloader.c"

using namespace std;

class PayloaderTest : public testing::Test
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

			temp_dir = getTempDir("PayloaderTest");
		}

		static void TearDownTestCase()
		{
			delete(gen);
			delete(dis);

			rmdir(temp_dir.c_str());
		}
};
mt19937_64* PayloaderTest::gen = nullptr;
uniform_int_distribution<uint8_t>* PayloaderTest::dis = nullptr;
string PayloaderTest::temp_dir;


TEST_F(PayloaderTest, testOverwriteInFile)
{
	uint64_t binary_size = 64;
	string src = temp_dir+"/testOverwriteInFile.bind";
//	string src = "/tmp/PayloaderTestSrc.tmp";
	vector<uint8_t> bytes = createBinary(src, binary_size);

	unsigned char pl[] = {
		0,0,222,173,190,239,0,0
	};
	payload = pl;
	payload_ln = sizeof(pl);
	snprintf(file_name, PATH_MAX, "%s", &src[0]);
	start = 10;
	file_size = getSize(file_name);

	cout << "start: "<<start<<endl;
	cout << "payload_ln: "<<payload_ln<<endl;
	cout << "file_name: "<<file_name<<endl;
	cout << "file_size: "<<file_size<<endl;

	overwrite();

	ifstream check_fs(src);
	check_fs.seekg(0);

	int j = 0;
	for ( int i = 0; i < binary_size; i++ )
	{
		unsigned char cs;
		check_fs.read(reinterpret_cast<char *>(&(cs)), 1);
		cout<<setw(2)<<setfill('0') << i <<hex<<" : "<<+cs<<  " = "<<+bytes[i]<<dec<<endl;

		if ( start <= i && i < start+payload_ln )
			EXPECT_EQ(cs, payload[j++]);
		else
			EXPECT_EQ(cs, bytes[i]);
	}

	remove(src.c_str());
}

TEST_F(PayloaderTest, testOverwriteOverEndOfFile)
{
	uint64_t binary_size = 8;
//	string src = temp_dir+"/testOverwriteOverEndOfFile.bind";
	string src = "/tmp/PayloaderTestSrc.tmp";
	vector<uint8_t> bytes = createBinary(src, binary_size);

	unsigned char pl[] = {
		0,0,222,173,190,239,0,0
	};
	payload = pl;
	payload_ln = sizeof(pl);
	snprintf(file_name, PATH_MAX, "%s", &src[0]);
	start = binary_size - payload_ln / 2;
	file_size = getSize(file_name);

	cout << "start: "<<start<<endl;
	cout << "payload_ln: "<<payload_ln<<endl;
	cout << "file_name: "<<file_name<<endl;
	cout << "file_size: "<<file_size<<endl;

	overwrite();

	ifstream check_fs(src);
	check_fs.seekg(0);

	int j = 0;
	for ( int i = 0; i < binary_size; i++ )
	{
		unsigned char cs;
		check_fs.read(reinterpret_cast<char *>(&(cs)), 1);
		cout<<setw(2)<<setfill('0') << i <<hex<<" : "<<+cs<<  " = "<<+bytes[i]<<dec<<endl;

		if ( start <= i && i < start+payload_ln )
			EXPECT_EQ(cs, payload[j++]);
		else
			EXPECT_EQ(cs, bytes[i]);
	}

	for ( int i = binary_size; i < binary_size + payload_ln / 2; i++ )
	{
		unsigned char cs;
		check_fs.read(reinterpret_cast<char *>(&(cs)), 1);
		cout<<setw(2)<<setfill('0') << i <<hex<<" : "<<+cs<<dec<<endl;

		EXPECT_EQ(cs, payload[j++]);
	}

	remove(src.c_str());
}

TEST_F(PayloaderTest, testOverwriteOutOfFile)
{
	uint64_t binary_size = 8;
//	string src = temp_dir+"/testOverwriteOutOfFile.bind";
	string src = "/tmp/PayloaderTestSrc.tmp";
	vector<uint8_t> bytes = createBinary(src, binary_size);

	unsigned char pl[] = {
		255,255,222,173,190,239,0,0
	};
	payload = pl;
	payload_ln = sizeof(pl);
	snprintf(file_name, PATH_MAX, "%s", &src[0]);
	start = binary_size + 2;
	file_size = getSize(file_name);

	cout << "start: "<<start<<endl;
	cout << "payload_ln: "<<payload_ln<<endl;
	cout << "file_name: "<<file_name<<endl;
	cout << "file_size: "<<file_size<<endl;

	overwrite();

	ifstream check_fs(src);
	check_fs.seekg(0);

	for ( int i = 0; i < binary_size; i++ )
	{
		unsigned char cs;
		check_fs.read(reinterpret_cast<char *>(&(cs)), 1);
		cout<<setw(2)<<setfill('0') << i <<hex<<" : "<<+cs<<  " = "<<+bytes[i]<<dec<<endl;

		EXPECT_EQ(cs, bytes[i]);
	}

	int j = 0;
	check_fs.seekg(start);
	for ( int i = start; i < start + payload_ln; i++ )
	{
		unsigned char cs;
		check_fs.read(reinterpret_cast<char *>(&(cs)), 1);
		cout<<setw(2)<<setfill('0') << i <<hex<<" : "<<+cs<<dec<<endl;

		EXPECT_EQ(cs, payload[j++]);
	}

	remove(src.c_str());
}

TEST_F(PayloaderTest, testInsertInFile)
{
	uint64_t binary_size = 8;
	string src = temp_dir+"/testInsertInFile.hex";
//	string src = "/tmp/testInsertInFile.tmp";
	vector<uint8_t> bytes = createBinary(src, binary_size);

	unsigned char pl[] = {
			222,173,11,234
	};
	payload = pl;
	payload_ln = sizeof(pl);
	snprintf(file_name, PATH_MAX, "%s", &src[0]);
	start = 2;
	file_size = getSize(file_name);

	vector<uint8_t> payloaded_bytes(bytes.begin(), bytes.end());
	for ( int i = 0; i < payload_ln; i++ )
		payloaded_bytes.insert(payloaded_bytes.begin() + (start + i), pl[i]);

	insert();

	ifstream check_fs(src);
	uint64_t size = getSize(file_name);
	cout << " new size: "<<size<<endl;
	EXPECT_EQ(size, binary_size + payload_ln);
	check_fs.seekg(0);
//
	for ( int i = 0; i < size; i++ )
	{
		unsigned char cs;
		check_fs.read(reinterpret_cast<char *>(&(cs)), 1);
//		cout<<setw(2)<<setfill('0') << i <<hex<<" : "<<+cs<<  " = "<<+payloaded_bytes[i]<<dec<<endl;

		EXPECT_EQ(cs, payloaded_bytes[i]);
	}
//
	check_fs.close();
	remove(src.c_str());
}

TEST_F(PayloaderTest, testInsertOverFileBounds)
{
	uint64_t binary_size = 8;
//	string src = temp_dir+"/testInsertInFile.hex";
	string src = "/tmp/testInsertInFile.tmp";
	vector<uint8_t> bytes = createBinary(src, binary_size);

	unsigned char pl[] = {
			222,173,11,234
	};
	payload = pl;
	payload_ln = sizeof(pl);
	snprintf(file_name, PATH_MAX, "%s", &src[0]);
	start = 6;
	file_size = getSize(file_name);

//	cout << "start: "<<start<<endl;
//	cout << "payload_ln: "<<payload_ln<<endl;
//	cout << "file_name: "<<file_name<<endl;
//	cout << "file_size: "<<file_size<<endl;

	vector<uint8_t> payloaded_bytes(bytes.begin(), bytes.end());
	for ( int i = 0; i < payload_ln; i++ )
	{
		payloaded_bytes.insert(payloaded_bytes.begin() + (start + i), pl[i]);
	}
	for ( uint8_t p : bytes )
		cout << hex << +p << "|";
	cout << endl;
	for ( uint8_t p : payloaded_bytes )
		cout << hex << +p << "|";
	cout << endl;

	insert();

	ifstream check_fs(src);
	uint64_t size = getSize(file_name);
	cout << " new size: "<<size<<endl;
	EXPECT_EQ(size, binary_size + payload_ln);
	check_fs.seekg(0);
//
	for ( int i = 0; i < size; i++ )
	{
		unsigned char cs;
		check_fs.read(reinterpret_cast<char *>(&(cs)), 1);
		cout<<setw(2)<<setfill('0') << i <<hex<<" : "<<+cs<<  " = "<<+payloaded_bytes[i]<<dec<<endl;

		EXPECT_EQ(cs, payloaded_bytes[i]);
	}
//
	check_fs.close();
//	remove(src.c_str());
}

#endif
