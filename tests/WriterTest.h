#ifndef G_TESTS_WRITER_TEST_H
#define G_TESTS_WRITER_TEST_H

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
#include <functional>

#include "misc/Misc.h"
#include "../src/utils/common_fileio.c"
#include "../src/utils/Helper.c"
#include "../src/Globals.h"
#define BLOCKSIZE_LARGE 0x10
#include "../src/Writer.c"

using namespace std;

class WriterTest : public testing::Test
{
	protected:
		static string temp_dir;

		static Misc misc;

//		std::random_device rd;
//		static mt19937_64* gen;
//		static uniform_int_distribution<uint8_t>* dis;

		using PayloadParser = function<uint32_t(const char*, unsigned char**)>;
		using TV = tuple<const char*, uint32_t, vector<uint8_t>>;

		void assertPayloadParser(vector<TV>& tv, PayloadParser p)
		{
			for ( TV e : tv )
			{
				unsigned char* parsed = nullptr;
				uint32_t payload_ln = p(get<0>(e), &parsed);

				EXPECT_EQ(payload_ln, get<1>(e));

				vector<uint8_t> expected = get<2>(e);
				if ( expected.empty() )
				{
					EXPECT_EQ(parsed, nullptr);
				}
				else
				{
					for ( int i = 0; i < expected.size(); i++ )
					{
						EXPECT_EQ(parsed[i], expected[i]);
					}
				}
			}
		}

		static string getTempDir(const std::string& prefix)
		{
			string tmp = "/tmp/"+prefix+"XXXXXX";
			char* buf = &tmp[0];
			char* dir = mkdtemp(buf);

			return string(dir);
		}

//		vector<uint8_t> createBinary(const string& file_src, size_t f_size)
//		{
//			ofstream f;
//			vector<uint8_t> values;
//			values.resize(f_size);
//
//			f.open(file_src, ios::binary | std::ios::out);
//			f.clear();
//
//			for ( size_t i = 0; i < f_size; i++ )
//			{
//				uint8_t value = (*dis)(*gen);
//				uint8_t size = sizeof(value);
//				f.write(reinterpret_cast<char *>(&(value)), size);
//				values[i] = value;
//			}
//
//			f.close();
//
//			return values;
//		}

	public:
		static void SetUpTestCase()
		{
//			std::random_device rd;
//			gen = new mt19937_64(rd());
//			dis = new uniform_int_distribution<uint8_t>(0, UINT8_MAX);

			temp_dir = getTempDir("WriterTest");
		}

		static void TearDownTestCase()
		{
//			delete(gen);
//			delete(dis);

			rmdir(temp_dir.c_str());
		}
};
//mt19937_64* WriterTest::gen = nullptr;
//uniform_int_distribution<uint8_t>* WriterTest::dis = nullptr;
string WriterTest::temp_dir;
Misc WriterTest::misc;


TEST_F(WriterTest, testOverwriteInFile)
{
	uint64_t binary_size = 64;
	string src = temp_dir+"/testOverwriteInFile.bind";
//	string src = "/tmp/WriterTestSrc.tmp";
	vector<uint8_t> bytes = misc.createBinary(src, binary_size);

	unsigned char pl[] = {
		0,0,222,173,190,239,0,0
	};
	unsigned char* payload = pl;
	uint32_t payload_ln = sizeof(pl);
	snprintf(file_path, PATH_MAX, "%s", &src[0]);
	start = 10;
	file_size = getSize(file_path);

	cout << "start: "<<start<<endl;
	cout << "payload_ln: "<<payload_ln<<endl;
	cout << "file_path: "<<file_path<<endl;
	cout << "file_size: "<<file_size<<endl;

	overwrite(payload, payload_ln);

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

TEST_F(WriterTest, testOverwriteOverEndOfFile)
{
	uint64_t binary_size = 8;
//	string src = temp_dir+"/testOverwriteOverEndOfFile.bind";
	string src = "/tmp/WriterTestSrc.tmp";
	vector<uint8_t> bytes = misc.createBinary(src, binary_size);

	unsigned char pl[] = {
		0,0,222,173,190,239,0,0
	};
	unsigned char* payload = pl;
	uint32_t payload_ln = sizeof(pl);
	snprintf(file_path, PATH_MAX, "%s", &src[0]);
	start = binary_size - payload_ln / 2;
	file_size = getSize(file_path);

	overwrite(payload, payload_ln);

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

TEST_F(WriterTest, testOverwriteOutOfFile)
{
	uint64_t binary_size = 64;
//	string src = temp_dir+"/testOverwriteOutOfFile.bind";
	string src = "/tmp/WriterTestSrc.tmp";
	vector<uint8_t> bytes = misc.createBinary(src, binary_size);

	unsigned char pl[] = {
		255,255,222,173,190,239,0,0
	};
	unsigned char* payload = pl;
	uint32_t payload_ln = sizeof(pl);
	snprintf(file_path, PATH_MAX, "%s", &src[0]);
	start = binary_size + 2;
	file_size = getSize(file_path);

	cout << "start: "<<start<<endl;
	cout << "payload_ln: "<<payload_ln<<endl;
	cout << "file_path: "<<file_path<<endl;
	cout << "file_size: "<<file_size<<endl;

	overwrite(payload, payload_ln);

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

TEST_F(WriterTest, testInsertInFile)
{
	uint64_t binary_size = 64;
	string src = temp_dir+"/testInsertInFile.hex";
//	string src = "/tmp/testInsertInFile.tmp";
	vector<uint8_t> bytes = misc.createBinary(src, binary_size);

	unsigned char pl[] = {
			222,173,11,234
	};
	unsigned char* payload = pl;
	uint32_t payload_ln = sizeof(pl);
	snprintf(file_path, PATH_MAX, "%s", &src[0]);
	start = 2;
	file_size = getSize(file_path);

	vector<uint8_t> payloaded_bytes(bytes.begin(), bytes.end());
	for ( int i = 0; i < payload_ln; i++ )
		payloaded_bytes.insert(payloaded_bytes.begin() + (start + i), pl[i]);

	insert(payload, payload_ln);

	ifstream check_fs(src);
	uint64_t size = getSize(file_path);
//	cout << " new BLOCKSIZE_LARGE: "<<BLOCKSIZE_LARGE<<endl;
//	cout << " new size: "<<size<<endl;
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

TEST_F(WriterTest, testInsertOverFileBounds)
{
	uint64_t binary_size = 64;
	string src = temp_dir+"/testInsertOverFileBounds.hex";
//	string src = "/tmp/testInsertOverFileBounds.tmp";
	vector<uint8_t> bytes = misc.createBinary(src, binary_size);

	unsigned char pl[] = {
			222,173,11,234
	};
	unsigned char* payload = pl;
	uint32_t payload_ln = sizeof(pl);
	snprintf(file_path, PATH_MAX, "%s", &src[0]);
	start = binary_size - 2;
	file_size = getSize(file_path);

//	cout << " new BLOCKSIZE_LARGE: "<<BLOCKSIZE_LARGE<<endl;
//	cout << "start: "<<start<<endl;
//	cout << "payload_ln: "<<payload_ln<<endl;
//	cout << "file_path: "<<file_path<<endl;
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

	insert(payload, payload_ln);

	ifstream check_fs(src);
	uint64_t size = getSize(file_path);
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
	remove(src.c_str());
}

TEST_F(WriterTest, testInsertOutOfFileBounds)
{
	uint64_t binary_size = 64;
	string src = temp_dir+"/testInsertOutOfFileBounds.hex";
//	string src = "/tmp/testInsertOutOfFileBounds.tmp";
	vector<uint8_t> bytes = misc.createBinary(src, binary_size);

	unsigned char pl[] = {
			222,173,11,234
	};
	unsigned char* payload = pl;
	uint32_t payload_ln = sizeof(pl);
	snprintf(file_path, PATH_MAX, "%s", &src[0]);
	start = binary_size + 2;
	file_size = getSize(file_path);

//	cout << " new BLOCKSIZE_LARGE: "<<BLOCKSIZE_LARGE<<endl;
	vector<uint8_t> payloaded_bytes(bytes.begin(), bytes.end());
	for ( int i = 0; i < start-bytes.size(); i++ )
		payloaded_bytes.push_back(0);
	for ( int i = 0; i < payload_ln; i++ )
		payloaded_bytes.push_back(pl[i]);

	for ( uint8_t p : bytes )
		cout << hex << +p << "|";
	cout << endl;
	for ( uint8_t p : payloaded_bytes )
		cout << hex << +p << "|";
	cout << endl;

	insert(payload, payload_ln);

	ifstream check_fs(src);
	uint64_t size = getSize(file_path);
	cout << " new size: "<<size<<endl;
	EXPECT_EQ(size, start + payload_ln);
	check_fs.seekg(0);

	for ( int i = 0; i < size; i++ )
	{
		unsigned char cs;
		check_fs.read(reinterpret_cast<char *>(&(cs)), 1);
		cout<<setw(2)<<setfill('0') << i <<hex<<" : "<<+cs<<  " = "<<+payloaded_bytes[i]<<dec<<endl;

		EXPECT_EQ(cs, payloaded_bytes[i]);
	}

	check_fs.close();
	remove(src.c_str());
}

TEST_F(WriterTest, testDelete)
{
	uint64_t binary_size = 64;
//	string src = temp_dir+"/testDelete.hex";
	string src = "/tmp/testDelete.tmp";
	vector<uint8_t> bytes = misc.createBinary(src, binary_size);

	snprintf(file_path, PATH_MAX, "%s", &src[0]);
	start = 2;
	length = 8;
	file_size = getSize(file_path);

//	cout << " new BLOCKSIZE_LARGE: "<<BLOCKSIZE_LARGE<<endl;
	vector<uint8_t> deleted_bytes = bytes;
	deleted_bytes.erase(deleted_bytes.begin()+start, deleted_bytes.begin()+start+length);

	cout << "bytes:"<<endl;
	for ( uint8_t p : bytes )
		cout << hex <<setw(2)<<setfill('0')<< +p << "|";
	cout << endl;
	cout << "deleted bytes:"<<endl;
	for ( uint8_t p : deleted_bytes )
		cout << hex <<setw(2)<<setfill('0')<< +p << "|";
	cout << endl;

	deleteBytes(start, length);

	ifstream check_fs(src);
	uint64_t size = getSize(file_path);
	cout << " new size: "<<dec<<size<<endl;
	EXPECT_EQ(size, binary_size-length);
	check_fs.seekg(0);

	for ( int i = 0; i < size; i++ )
	{
		unsigned char cs;
		check_fs.read(reinterpret_cast<char *>(&(cs)), 1);
		cout<<setw(2)<<setfill('0') << i <<hex<<" g: "<<setw(2)<<setfill('0')<<+cs<<  " = "<<setw(2)<<setfill('0')<<+deleted_bytes[i]<<dec<<" :e"<<endl;

		EXPECT_EQ(cs, deleted_bytes[i]);
	}

	check_fs.close();
	remove(src.c_str());
}

TEST_F(WriterTest, testParsePlainBytes)
{
	const char* arg0 = "dead0bea";
	const char* arg1 = "";
	const char* arg2 = "ead0bea";

	unsigned char* parsed0;
	unsigned char* parsed1 = nullptr;
	unsigned char* parsed2 = nullptr;

	uint32_t payload0_ln = payloadParsePlainBytes(arg0, &parsed0);
	unsigned char expected0[] = { 222, 173, 11, 234 };

	EXPECT_EQ(payload0_ln, 4);

	for ( int i = 0; i < 4; i++ )
		EXPECT_EQ(parsed0[i], expected0[i]);

	uint32_t payload1_ln = payloadParsePlainBytes(arg1, &parsed1);
	uint32_t payload2_ln = payloadParsePlainBytes(arg2, &parsed2);

	EXPECT_EQ(payload1_ln, 4);
	EXPECT_EQ(payload2_ln, 4);

	EXPECT_EQ(parsed1, nullptr);
	EXPECT_EQ(parsed2, nullptr);
}

TEST_F(WriterTest, testParseReversedPlainBytes)
{
	const char* arg0 = "dead0bea";
	const char* arg1 = "";
	const char* arg2 = "ead0bea";
	unsigned char* parsed0 = nullptr;
	unsigned char* parsed1 = nullptr;
	unsigned char* parsed2 = nullptr;

	uint32_t payload0_ln = payloadParseReversedPlainBytes(arg0, &parsed0);
	unsigned char expected[] = { 234, 11, 173, 222 };

	EXPECT_EQ(payload0_ln, 4);

	for ( int i = 0; i < 4; i++ )
		EXPECT_EQ(parsed0[i], expected[i]);

	uint32_t payload1_ln = payloadParseReversedPlainBytes(arg1, &parsed1);
	uint32_t payload2_ln = payloadParseReversedPlainBytes(arg2, &parsed2);

	EXPECT_EQ(payload1_ln, 0);
	EXPECT_EQ(payload2_ln, 0);

	EXPECT_EQ(parsed1, nullptr);
	EXPECT_EQ(parsed2, nullptr);
}

TEST_F(WriterTest, testParseString)
{
	const char* arg0 = "dead bea";
	const char* arg1 = "";
	const char* arg2 = "a";
	unsigned char* parsed0 = nullptr;
	unsigned char* parsed1 = nullptr;
	unsigned char* parsed2 = nullptr;

	uint32_t payload0_ln = payloadParseString(arg0, &parsed0);
	uint32_t payload1_ln = payloadParseString(arg1, &parsed1);
	uint32_t payload2_ln = payloadParseString(arg2, &parsed2);

	unsigned char expected0[] = { 100, 101, 97, 100, 32, 98, 101, 97 };
	uint32_t expexted0_ln = 8;
	unsigned char expected2[] = { 97 };
	uint32_t expexted2_ln = 1;

	EXPECT_EQ(payload0_ln, expexted0_ln);

	for ( int i = 0; i < expexted0_ln; i++ )
		EXPECT_EQ(parsed0[i], expected0[i]);

	for ( int i = 0; i < expexted2_ln; i++ )
		EXPECT_EQ(parsed2[i], expected2[i]);

	EXPECT_EQ(payload1_ln, 0);
	EXPECT_EQ(payload2_ln, 1);

	EXPECT_EQ(parsed1, nullptr);
}

TEST_F(WriterTest, testParseByte)
{
	using TV = tuple<const char*, uint32_t, vector<uint8_t>>;
	vector<TV> tv = {
			TV("DE", 1, {0xDE}),
			TV("E", 1, {0x0E}),
			TV("", 0, {}),
			TV("aaa", 0, {}),
	};

	assertPayloadParser(tv, payloadParseByte);
}

TEST_F(WriterTest, testParseWord)
{
	vector<TV> tv = {
			TV("DEEE", 2, {0xEE, 0xDE}),
			TV("E", 2, {0x0E, 0x00}),
			TV("", 0, {}),
			TV("aaeea", 0, {}),
	};

	assertPayloadParser(tv, payloadParseWord);
}

TEST_F(WriterTest, testParseDWord)
{
	using TV = tuple<const char*, uint32_t, vector<uint8_t>>;
	vector<TV> tv = {
			TV("DEADBEAF", 4, {0xAF, 0xBE, 0xAD, 0xDE}),
			TV("E", 4, {0x0E, 0x00, 0x00, 0x00}),
			TV("", 0, {}),
			TV("aaeeaaeea", 0, {}),
	};

	assertPayloadParser(tv, payloadParseDWord);
}

TEST_F(WriterTest, testParseQWord)
{
	using TV = tuple<const char*, uint32_t, vector<uint8_t>>;
	vector<TV> tv = {
			TV("fedcba9876543210", 8, {0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe}),
			TV("E", 8, {0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}),
			TV("", 0, {}),
			TV("aaeeaaeeaaeeaaeea", 0, {}),
	};

	assertPayloadParser(tv, payloadParseQWord);
}

#endif
