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
#include <filesystem>

#include "misc/Misc.h"
//#include "../src/Globals.h"
#define BLOCKSIZE_LARGE 0x10
//#include "../src/Writer.c"

using namespace std;

namespace fs = std::filesystem;

class WriterTest : public testing::Test
{
	protected:
		static string temp_dir;
		const string pe_file = "tests/files/qappsrv.exe";

		static Misc misc;

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
					for ( size_t i = 0; i < expected.size(); i++ )
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

		void assertProperlyDeleted(const string& src, vector<uint8_t>& bytes, uint64_t start);
		void checkBytes(const string& file0, const string& file1, size_t start_o, size_t start_c, size_t size);

	public:
		static void SetUpTestCase()
		{
			temp_dir = getTempDir("WriterTest");
		}

		static void TearDownTestCase()
		{
			rmdir(temp_dir.c_str());
		}
};
string WriterTest::temp_dir;
Misc WriterTest::misc;


TEST_F(WriterTest, testOverwriteInFile)
{
	uint64_t binary_size = 64;
	string src = temp_dir+"/testOverwriteInFile.bin";
//	string src = "/tmp/WriterTestSrc.tmp";
	vector<uint8_t> bytes = misc.createBinary(src, binary_size);

	unsigned char pl[] = {
		0,0,222,173,190,239,0,0
	};
	unsigned char* payload = pl;
	uint32_t payload_ln = sizeof(pl);
	snprintf(file_path, PATH_MAX, "%s", &src[0]);
	uint64_t start = 10;
	file_size = getSize(file_path);

	cout << "start: "<<start<<endl;
	cout << "payload_ln: "<<payload_ln<<endl;
	cout << "file_path: "<<file_path<<endl;
	cout << "file_size: "<<file_size<<endl;

	overwrite(&src[0], payload, payload_ln, start);

	ifstream check_fs(src);
	check_fs.seekg(0);

	int j = 0;
	for ( int i = 0; i < binary_size; i++ )
	{
		unsigned char cs;
		check_fs.read((char*)(&(cs)), 1);
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
	uint64_t start = binary_size - payload_ln / 2;
	file_size = getSize(file_path);

	overwrite(&src[0], payload, payload_ln, start);

	ifstream check_fs(src);
	check_fs.seekg(0);

	int j = 0;
	for ( int i = 0; i < binary_size; i++ )
	{
		unsigned char cs;
		check_fs.read((char*)(&(cs)), 1);
		cout<<setw(2)<<setfill('0') << i <<hex<<" : "<<+cs<<  " = "<<+bytes[i]<<dec<<endl;

		if ( start <= i && i < start+payload_ln )
			EXPECT_EQ(cs, payload[j++]);
		else
			EXPECT_EQ(cs, bytes[i]);
	}

	for ( int i = binary_size; i < binary_size + payload_ln / 2; i++ )
	{
		unsigned char cs;
		check_fs.read((char*)(&(cs)), 1);
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
	uint64_t start = binary_size + 2;
	file_size = getSize(file_path);

	cout << "start: "<<start<<endl;
	cout << "payload_ln: "<<payload_ln<<endl;
	cout << "file_path: "<<file_path<<endl;
	cout << "file_size: "<<file_size<<endl;

	overwrite(&src[0], payload, payload_ln, start);

	ifstream check_fs(src);
	check_fs.seekg(0);

	for ( int i = 0; i < binary_size; i++ )
	{
		unsigned char cs;
		check_fs.read((char*)(&(cs)), 1);
		cout<<setw(2)<<setfill('0') << i <<hex<<" : "<<+cs<<  " = "<<+bytes[i]<<dec<<endl;

		EXPECT_EQ(cs, bytes[i]);
	}

	int j = 0;
	check_fs.seekg(start);
	for ( uint64_t i = start; i < start + payload_ln; i++ )
	{
		unsigned char cs;
		check_fs.read((char*)(&(cs)), 1);
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
	uint64_t start = 2;
	file_size = getSize(file_path);

	vector<uint8_t> payloaded_bytes(bytes.begin(), bytes.end());
	for ( int i = 0; i < payload_ln; i++ )
		payloaded_bytes.insert(payloaded_bytes.begin() + (start + i), pl[i]);

	insert(&src[0], payload, payload_ln, start);

	ifstream check_fs(src);
	uint64_t size = getSize(file_path);
//	cout << " new BLOCKSIZE_LARGE: "<<BLOCKSIZE_LARGE<<endl;
//	cout << " new size: "<<size<<endl;
	EXPECT_EQ(size, binary_size + payload_ln);
	check_fs.seekg(0);
//
	for ( uint64_t i = 0; i < size; i++ )
	{
		unsigned char cs;
		check_fs.read((char*)(&(cs)), 1);
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
	uint64_t start = binary_size - 2;
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

	insert(&src[0], payload, payload_ln, start);

	ifstream check_fs(src);
	uint64_t size = getSize(file_path);
	cout << " new size: "<<size<<endl;
	EXPECT_EQ(size, binary_size + payload_ln);
	check_fs.seekg(0);
//
	for ( int i = 0; i < size; i++ )
	{
		unsigned char cs;
		check_fs.read((char*)(&(cs)), 1);
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
	uint64_t start = binary_size + 2;
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

	insert(&src[0], payload, payload_ln, start);

	ifstream check_fs(src);
	uint64_t size = getSize(file_path);
	cout << " new size: "<<size<<endl;
	EXPECT_EQ(size, start + payload_ln);
	check_fs.seekg(0);

	for ( int i = 0; i < size; i++ )
	{
		unsigned char cs;
		check_fs.read((char*)(&(cs)), 1);
		cout<<setw(2)<<setfill('0') << i <<hex<<" : "<<+cs<<  " = "<<+payloaded_bytes[i]<<dec<<endl;

		EXPECT_EQ(cs, payloaded_bytes[i]);
	}

	check_fs.close();
	remove(src.c_str());
}

TEST_F(WriterTest, testDeleteStart)
{
	uint64_t binary_size = 64;
//	string src = temp_dir+"/testDelete.hex";
	string src = "/tmp/testDelete.tmp";
	vector<uint8_t> bytes = misc.createBinary(src, binary_size);

	uint64_t start = 0;
	length = 17;

	assertProperlyDeleted(src, bytes, start);

	remove(src.c_str());
}

TEST_F(WriterTest, testDeleteMiddle)
{
	uint64_t binary_size = 65;
//	string src = temp_dir+"/testDelete.hex";
	string src = "/tmp/testDelete.tmp";
	vector<uint8_t> bytes = misc.createBinary(src, binary_size);

	uint64_t start = 12;
	length = 9;

	assertProperlyDeleted(src, bytes, start);

	remove(src.c_str());
}

TEST_F(WriterTest, testDeleteEnd)
{
	uint64_t binary_size = 64;
//	string src = temp_dir+"/testDelete.hex";
	string src = "/tmp/testDelete.tmp";
	vector<uint8_t> bytes = misc.createBinary(src, binary_size);

	uint64_t start = 31;
	length = binary_size - start;

	assertProperlyDeleted(src, bytes, start);

	remove(src.c_str());
}

void WriterTest::assertProperlyDeleted(const string& src, vector<uint8_t>& bytes, uint64_t start)
{
	snprintf(file_path, PATH_MAX, "%s", &src[0]);
	file_size = getSize(file_path);

//	cout << " new BLOCKSIZE_LARGE: "<<BLOCKSIZE_LARGE<<endl;
	vector<uint8_t> resulting_bytes = bytes;
	resulting_bytes.erase(resulting_bytes.begin() + start, resulting_bytes.begin() + start + length);
	vector<uint8_t> deleted_bytes(bytes.begin()+start, bytes.begin()+start+length);

	printf("bytes:\n");
	for ( uint8_t p : bytes )
		printf("%02x|", +p);
	printf("\n");
	printf("deleted_bytes bytes:\n");
	for ( uint8_t p : deleted_bytes  )
		printf("%02x|", +p);
	printf("\n");
	printf("resulting bytes:\n");
	for ( uint8_t p : resulting_bytes )
		printf("%02x|", +p);
	printf("\n");

	deleteBytes(&src[0], start, length);

	ifstream check_fs(src);
	uint64_t size = getSize(file_path);
	size_t expected_size = bytes.size()-length;
	printf(" old size: 0x%lx\n", bytes.size());
	printf(" new size: 0x%lx\n", size);
	printf(" exp size: 0x%lx\n", expected_size);
	EXPECT_EQ(size, expected_size);
	check_fs.seekg(0);

	for ( int i = 0; i < size; i++ )
	{
		unsigned char cs;
		check_fs.read((char*)(&(cs)), 1);
//		printf("0x%x g: %02x = %02x :e\n", i, +cs, deleted_bytes[i]);

		EXPECT_EQ(cs, resulting_bytes[i]);
	}

	check_fs.close();
}

void WriterTest::checkBytes(const string& file0, const string& file1, size_t start_o, size_t start_c, size_t size)
{
	uint8_t bo;
	uint8_t bc;
	ifstream original(file0);
	ifstream cropped(file1);

	original.seekg(start_o);
	cropped.seekg(start_c);

	for ( size_t i = 0; i < size; i++ )
	{
		original.read((char*)(&(bo)), 1);
		cropped.read((char*)(&(bc)), 1);

		EXPECT_EQ(bo, bc);
	}
	original.close();
	cropped.close();
}

TEST_F(WriterTest, testMultiDeleteOnRealFile)
{
	string src0 = pe_file;

	string file0 = "/tmp/testMultiDeleteOnRealFile.exe";

	fs::copy_file(src0, file0, fs::copy_options::skip_existing);

	uint64_t c0_s = 0x400; // coderegion start
	uint64_t c0_e = 0x4642; // coderegion end

	uint64_t file0_size_o = fs::file_size(file0);
	uint64_t file0_size = file0_size_o;

	uint64_t s = 0;
	uint64_t l = c0_s;

	snprintf(file_path, PATH_MAX, "%s", &file0[0]);

//	printf("file_size(file0): 0x%lx\n", file0_size);
//	printf("delete s: 0x%lx, l: 0x%lx\n", s, l);

	file_size = getSize(file_path);

	deleteBytes(&file0[0], s, l);

	file0_size = fs::file_size(file0);
	EXPECT_EQ(file0_size, file0_size_o - l);

//	printf("file_size(file0): 0x%lx\n", fs::file_size(file0));

	checkBytes(src0, file0, c0_s, s, file0_size);

//	printf("\n\n");
	s = c0_e - c0_s;
	l = file0_size - s;
	file_size = getSize(file_path);
//	printf("s: 0x%lx, l: 0x%lx\n", s, l);

	deleteBytes(&file0[0], s, l);
	file0_size = fs::file_size(file0);
	EXPECT_EQ(file0_size, file0_size_o - l - c0_s);

//	printf("file_size(file0): 0x%lx\n", fs::file_size(file0));

	checkBytes(src0, file0, c0_s, 0, file0_size);

	fs::remove(file0);
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

	EXPECT_EQ(payload1_ln, 0);
	EXPECT_EQ(payload2_ln, 0);

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
