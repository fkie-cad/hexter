#ifndef G_TESTS_HEXTER_TEST_H
#define G_TESTS_HEXTER_TEST_H

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

#include "misc/Misc.h"
#include "../src/Globals.h"
#include "../src/utils/Helper.h"
//#include "../src/hexter.c"
#define BLOCKSIZE_LARGE 0x400

using namespace std;

class HexterTest :public testing::Test
{
	protected:
		const string prog_dir = "./";
		const string prog = "hexter";
		static string temp_dir;
		const string vs = "1.5.2";

		static Misc misc;

//		std::random_device rd;
//		static mt19937_64* gen;
//		static uniform_int_distribution<uint8_t>* dis;

		enum PrintType
		{
			HEX, ASCII, DOUBLE, TRIPLE
		};

		const vector<string> missing_args_lines = {
				"Usage: ./" + prog + " filename [options]",
				"Usage: ./" + prog + " [options] filename",
				"Version: " + vs,
		};

		const vector<string> help_lines = {
				"Usage: ./" + prog + " filename [options]",
				"Usage: ./" + prog + " [options] filename",
				"Version: " + vs,
				"Options:",
//				" * -s:uint64_t Startoffset. Default = 0.",
//				" * -l:uint64_t Length of the part to display. Default = 50.",
//				" * -a ASCII only print.",
//				" * -x HEX only print.",
//				" * -c Clean output (no text formating in the console).",
//				" * -ix Insert hex byte sequence (destructive!). Where x is an format option.",
//				" * -ox Overwrite hex byte sequence (destructive!). Where x is an format option.",
//				" * -fx Find hex byte sequence. Where x is an format option.",
//				" * * Format options: h: plain bytes, a: ascii text, b: byte, w: word, d: double word, q: quad word).",
//				"     Expect for the ascii string, all values have to be passed as hex values.",
//				" * -d Delete -l bytes from -s.",
//				" * -h Print this.",
//				"",
//				"Example: "+prog_dir+prog+" path/to/a.file -s 100 -l 128 -x",
//				"Example: "+prog_dir+prog+" path/to/a.file -ih dead -s 0x100",
//				"Example: "+prog_dir+prog+" path/to/a.file -oh 0bea -s 0x100",
//				"Example: "+prog_dir+prog+" path/to/a.file -fh f001 -s 0x100",
//				"Example: "+prog_dir+prog+" path/to/a.file -d -s 0x100 -l 0x8",
//				"",
		};

		const vector<string> unknown_args_lines = {
				"INFO: Unknown arg type \"-x0\"",
				"File a does not exist.",
		};

		const vector<string> not_passed_value_args_lines = {
				"INFO: Arg \"-s\" has no value! Skipped!",
				"File a does not exist.",
		};

		const vector<string> incompatible_args_lines = {
				"ERROR: overwrite, insert, delete and find have to be used exclusively!",
		};

		static string getTempDir(const std::string& prefix)
		{
			string tmp = "/tmp/" + prefix + "XXXXXX";
			char* buf = &tmp[0];
			char* dir = mkdtemp(buf);

			return string(dir);
		}

		string createCommand(const string& args) const
		{
			stringstream ss;
			ss << prog_dir << prog << " " << args.substr(0, 500) << " 2>&1";

			return ss.str();
		}

		int openFile(const string& command, FILE*& fi) const
		{
			int errsv = errno;
			errno = 0;
			fi = popen(&command[0], "r");

			return errsv;
		}

		vector<string> getResult(FILE* fi)
		{
			char raw_line[200] = {0};
			vector<string> lines;

			while ( fgets(raw_line, sizeof(raw_line), fi) != NULL )
			{
				if ( strlen(raw_line))
				{
					string line = string(raw_line);
					line = line.substr(0, line.size() - 1);
					lines.emplace_back(line);
				}
			}

			return lines;
		}

		void callApp(const vector<string>& argv, const vector<string>& expected_lines, Bool resize = false)
		{
			string args;
			for ( const string& a : argv ) args = args.append(a).append(" ");
			string command = createCommand(args);
			FILE* fi = nullptr;
			openFile(command, fi);
			vector<string> lines = getResult(fi);

			if ( resize )
				lines.resize(expected_lines.size());

//			cout << "lines"<<endl;
//			for ( const string& s : lines )
//				cout << s << endl;

			fclose(fi);
			EXPECT_EQ(lines, expected_lines);
		}

		vector<string> getExpectedLines(const string& name, vector<uint8_t> bytes, PrintType type, uint64_t start,
										uint64_t length);

		void getDoubleCols(vector<uint8_t>& block, vector<string>& lines);

		void getTripleCols(vector<uint8_t>& block, vector<string>& lines, uint64_t start, uint64_t end);

		void getHexCols(vector<uint8_t>& block, vector<string>& lines);

		void getAsciiCols(vector<uint8_t>& block, vector<string>& lines, uint8_t col_size);

		void fillOffsetCol(uint64_t offset, uint8_t width, stringstream& ss);

		uint64_t fillHexCol(uint64_t i, vector<uint8_t>& block, stringstream& ss);

		void fillAsciiCol(uint64_t i, vector<uint8_t>& block, stringstream& ss, uint8_t col_size) const;

	public:

		static void SetUpTestCase()
		{
			std::random_device rd;
//			gen = new mt19937_64(rd());
//			dis = new uniform_int_distribution<uint8_t>(0, UINT8_MAX);

			temp_dir = getTempDir("HexterTest");
		}

		static void TearDownTestCase()
		{
//			delete(gen);
//			delete(dis);

			rmdir(temp_dir.c_str());
		}
};

//mt19937_64* HexterTest::gen = nullptr;
//uniform_int_distribution<uint8_t>* HexterTest::dis = nullptr;
string HexterTest::temp_dir;
Misc HexterTest::misc;

TEST_F(HexterTest, testMainWithoutArgs)
{
	const vector<string> argv = {""};

	callApp(argv, missing_args_lines);
}

TEST_F(HexterTest, testMainWithFalseFormatedArgs)
{
	const vector<string> argv_x0 = {"-x0 a"};
	const vector<string> argv_x1 = {"a -x0"};
	const vector<string> argv_s = {"a -s"};

	callApp(argv_x0, unknown_args_lines);
	callApp(argv_x1, unknown_args_lines);
	callApp(argv_s, not_passed_value_args_lines);
}

TEST_F(HexterTest, testMainWithIncompatibleArgs)
{
	const vector<string> argv_fo = {"a -fh 10 -oh 10"};
	const vector<string> argv_fi = {"a -fh 10 -ih 10"};
	const vector<string> argv_io = {"a -ih 10 -d"};

	callApp(argv_fo, incompatible_args_lines);
	callApp(argv_fi, incompatible_args_lines);
	callApp(argv_io, incompatible_args_lines);
}

TEST_F(HexterTest, testMainWithHelpArg)
{
	const vector<string> argv = {"-h a"};

	callApp(argv, help_lines, true);
}

TEST_F(HexterTest, testMainWithNotExistingFile)
{
	string src = "not/ex/ist.ing";
	const vector<string> argv = {src};

	callApp(argv, {"File " + src + " does not exist."});
}

TEST_F(HexterTest, testMainWithRandomFile)
{
	uint64_t binary_size = DEFAULT_LENGTH;
	string name = "testMainWithRandomFile.bin";
	string src = temp_dir + "/" + name;
	vector<uint8_t> bytes = misc.createBinary(src, binary_size);
	const vector<string> argv = {src};

	vector<string> expected_lines = getExpectedLines(name, bytes, TRIPLE, 0, DEFAULT_LENGTH);

	callApp(argv, expected_lines);

	remove(src.c_str());
}

TEST_F(HexterTest, testMainWithRandomFileNegativeArgs)
{
	uint64_t binary_size = 128;
	string src = temp_dir + "/testMainWithRandomFile.bin";
	vector<uint8_t> bytes = misc.createBinary(src, binary_size);

	const vector<string> argv1 = {src, "-s -10", "-l -100"};
	vector<string> expected_lines1 = {"Error: -10 could not be converted to a number: is negative!",
									  "Error: -100 could not be converted to a number: is negative!",
									  "INFO: Could not parse start. Setting it to 0!",
									  "INFO: Could not parse length. Setting it to 256!",
									  "Info: Start offset 0 plus length 256 is greater then the file size 128",
									  "Printing only to file size."};
	callApp(argv1, expected_lines1, true);

	const vector<string> argv2 = {src, "-s 10", "-l -100"};
	vector<string> expected_lines2 = {"Error: -100 could not be converted to a number: is negative!",
									  "INFO: Could not parse length. Setting it to 256!",
									  "Info: Start offset 10 plus length 256 is greater then the file size 128",
									  "Printing only to file size."};
	callApp(argv2, expected_lines2, true);

	const vector<string> argv5 = {src, "-s 10", "-l -100 -b"};
	vector<string> expected_lines5 = {"Error: -100 could not be converted to a number: is negative!",
									  "INFO: Could not parse length. Setting it to 256!",
									  "Info: Start offset 10 plus length 256 is greater then the file size 128",
									  "Printing only to file size."};
	callApp(argv5, expected_lines5, true);

	const vector<string> argv3 = {src, "-s dd", "-l -100"};
	vector<string> expected_lines3 = {"Error: dd could not be converted to a number: Not a number!",
									  "Error: -100 could not be converted to a number: is negative!",
									  "INFO: Could not parse start. Setting it to 0!",
									  "INFO: Could not parse length. Setting it to 256!",
									  "Info: Start offset 0 plus length 256 is greater then the file size 128",
									  "Printing only to file size."};
	callApp(argv3, expected_lines3, true);

	const vector<string> argv4 = {src, "-s 10", "-l ff"};
	vector<string> expected_lines4 = {"Error: ff could not be converted to a number: Not a number!",
									  "INFO: Could not parse length. Setting it to 256!",
									  "Info: Start offset 10 plus length 256 is greater then the file size 128",
									  "Printing only to file size."};
	callApp(argv4, expected_lines4, true);

	remove(src.c_str());
}

TEST_F(HexterTest, testMainWithRandomFileCustomParams)
{
	uint64_t binary_size = 0x200;
	string name = "testMainWithRandomFileCustomParams.bin";
	string src = temp_dir + "/" + name;
	vector<uint8_t> bytes = misc.createBinary(src, binary_size);
	const vector<string> argv = {src, "-s 0x10", "-l 0x100", "-b"};

	vector<string> expected_lines = getExpectedLines(name, bytes, TRIPLE, 0x10, 0x100);

	callApp(argv, expected_lines);

	remove(src.c_str());
}

TEST_F(HexterTest, testMainWithRandomFileHexOnlyPrint)
{
	uint64_t binary_size = 0x100;
	string name = "testMainWithRandomFileHex.bin";
	string src = temp_dir + "/" + name;
	vector<uint8_t> bytes = misc.createBinary(src, binary_size);
	const vector<string> argv = {src, "-x", "-b"};

	vector<string> expected_lines = getExpectedLines(name, bytes, HEX, 0x0, DEFAULT_LENGTH);

	callApp(argv, expected_lines);

	remove(src.c_str());
}

TEST_F(HexterTest, testMainWithRandomFileAsciiOnlyPrint)
{
	uint64_t binary_size = 0x100;
	string name = "testMainWithRandomFileAscii.bin";
	string src = temp_dir + "/" + name;
	vector<uint8_t> bytes = misc.createBinary(src, binary_size);
	const vector<string> argv = {src, "-a", "-b"};

	vector<string> expected_lines = getExpectedLines(name, bytes, ASCII, 0x0, DEFAULT_LENGTH);

	callApp(argv, expected_lines);

	remove(src.c_str());
}

vector<string>
HexterTest::getExpectedLines(const string& name, vector<uint8_t> bytes, PrintType type, uint64_t start, uint64_t length)
{
	if ( start + length > bytes.size())
	{
		length = bytes.size() - start;
	}
	uint64_t end = start + length;
	uint16_t block_size = BLOCKSIZE_LARGE;
	uint64_t block_start = start;
	uint64_t read_size = 0;
	uint64_t parts = length / block_size;
	if ( length % block_size != 0 ) parts++;

	vector<string> lines;
	lines.emplace_back("file: " + name);

	for ( uint64_t p = 0; p < parts; p++ )
	{
		read_size = block_size;
		if ( block_start + read_size > end ) read_size = end - block_start;

		vector<uint8_t> block(&bytes[block_start], &bytes[block_start + read_size]);

		if ( type == HEX )
			getHexCols(block, lines);
		else if ( type == ASCII )
			getAsciiCols(block, lines, ASCII_COL_SIZE);
		else if ( type == DOUBLE )
			getDoubleCols(block, lines);
		else if ( type == TRIPLE )
			getTripleCols(block, lines, start, end);

		block_start += block_size;
	}

	return lines;
}

void HexterTest::getDoubleCols(vector<uint8_t>& block, vector<string>& lines)
{
	uint64_t k = 0;

	for ( uint64_t i = 0; i < block.size(); i += DOUBLE_COL_SIZE )
	{
		stringstream ss;
		k = fillHexCol(i, block, ss);

		uint8_t gap = DOUBLE_COL_SIZE - k;
		if ( gap > 0 )
		{
			for ( k = 0; k < gap; k++ )
			{
				ss << "   ";
			}
		}

		ss << COL_SEPARATOR << " ";

		fillAsciiCol(i, block, ss, DOUBLE_COL_SIZE);

		lines.emplace_back(ss.str());
	}
}

void HexterTest::getTripleCols(vector<uint8_t>& block, vector<string>& lines, uint64_t start, uint64_t end)
{
	uint64_t k = 0;
	uint8_t offset_width = countHexWidth64(end);

	for ( uint64_t i = 0; i < block.size(); i += DOUBLE_COL_SIZE )
	{
		stringstream ss;
		fillOffsetCol(start + i, offset_width, ss);
		k = fillHexCol(i, block, ss);

		uint8_t gap = DOUBLE_COL_SIZE - k;
		if ( gap > 0 )
		{
			for ( k = 0; k < gap; k++ )
			{
				ss << "   ";
			}
		}

		ss << COL_SEPARATOR << " ";

		fillAsciiCol(i, block, ss, DOUBLE_COL_SIZE);

		lines.emplace_back(ss.str());
	}
}

void HexterTest::fillOffsetCol(uint64_t offset, uint8_t width, stringstream& ss)
{
	ss << hex << setw(width) << setfill('0') << offset << ": ";
}

void HexterTest::getAsciiCols(vector<uint8_t>& block, vector<string>& lines, uint8_t col_size)
{
	for ( uint64_t i = 0; i < block.size(); i += col_size )
	{
		stringstream ss;
		fillAsciiCol(i, block, ss, col_size);

		lines.emplace_back(ss.str());
	}
}

void HexterTest::fillAsciiCol(uint64_t i, vector<uint8_t>& block, stringstream& ss, uint8_t col_size) const
{
	uint64_t temp_i;
	for ( uint64_t k = 0; k < col_size; k++ )
	{
		temp_i = i + k;
		if ( temp_i >= block.size())
			break;

		char c = block[temp_i];
		if ( MIN_PRINTABLE_ASCII_RANGE <= c && c <= MAX_PRINTABLE_ASCII_RANGE )
			ss << c;
		else
			ss << NO_PRINT_ASCII_SUBSTITUTION;
	}
}

void HexterTest::getHexCols(vector<uint8_t>& block, vector<string>& lines)
{
	for ( uint64_t i = 0; i < block.size(); i += HEX_COL_SIZE )
	{
		stringstream ss;
		fillHexCol(i, block, ss);

		lines.emplace_back(ss.str());
	}
}

uint64_t HexterTest::fillHexCol(uint64_t i, vector<uint8_t>& block, stringstream& ss)
{
	uint64_t temp_i;
	uint64_t k;

	for ( k = 0; k < HEX_COL_SIZE; k++ )
	{
		temp_i = i + k;
		if ( temp_i >= block.size())
			break;

		ss.width(2);
		ss.fill('0');
		ss << hex << uppercase << +block[temp_i] << " ";
	}

	return k;
}

TEST_F(HexterTest, testOverwrite)
{
	uint64_t binary_size = 0x50;
	string name = "testOverwrite.bla";
	string src = temp_dir + "/" + name;
	vector<uint8_t> bytes = misc.createBinary(src, binary_size);
	const vector<string> argv = {src, "-oh 0000dead0bea0000", "-x", "-l 0x50"};
	uint32_t start = 0x0;
	uint32_t length = 0x50;

	bytes[0] = '\x00';
	bytes[1] = '\x00';
	bytes[2] = '\xde';
	bytes[3] = '\xad';
	bytes[4] = '\x0b';
	bytes[5] = '\xea';
	bytes[6] = '\x00';
	bytes[7] = '\x00';

	vector<string> expected_lines = getExpectedLines(name, bytes, HEX, start, length);

//	cout << "expected_lines"<<endl;
//	for ( const string& s : expected_lines )
//		cout << s << endl;

	callApp(argv, expected_lines);

	remove(src.c_str());
}

TEST_F(HexterTest, testInsert)
{
	uint64_t binary_size = 0x60;
	string name = "testInsert.bla";
	string src = temp_dir + "/" + name;
	vector<uint8_t> bytes = misc.createBinary(src, binary_size);
	uint32_t start = 0x0;
	uint32_t length = 0x50;
	unsigned char pl[] = {
			0, 0, 222, 173, 11, 234, 0, 0
	};
	uint32_t payload_ln = 8;
	vector<uint8_t> payloaded_bytes(bytes.begin(), bytes.end());
	stringstream pl_ss;
	for ( int i = 0; i < payload_ln; i++ )
	{
		payloaded_bytes.insert(payloaded_bytes.begin() + (start + i), pl[i]);
		pl_ss << hex << setw(2) << setfill('0') << +pl[i];
	}
	const vector<string> argv = {src, "-ih " + pl_ss.str(), "-s " + to_string(start), "-l " + to_string(length), "-x"};

	vector<string> expected_lines = getExpectedLines(name, payloaded_bytes, HEX, start, length);

//	cout << "expected_lines"<<endl;
//	for ( const string& s : expected_lines )
//		cout << s << endl;

	callApp(argv, expected_lines);

	remove(src.c_str());
}

TEST_F(HexterTest, testFind)
{
//	uint64_t binary_size = 0x60;
	string name = "testFind.bla";
	string src = temp_dir + "/" + name;

	vector<uint8_t> bytes = {1, 1, 3, 4, 5, 6, 7, 8, 1, 2, 2, 4, 5, 6, 7, 8,
							 1, 2, 3, 3, 5, 6, 7, 8, 1, 2, 3, 4, 4, 6, 7, 8,
							 1, 2, 3, 4, 5, 5, 7, 8, 1, 2, 3, 4, 5, 6, 6, 8,
							 1, 2, 3, 4, 5, 6, 7, 7, 1, 2, 3, 4, 5, 6, 7, 8,
							 1, 2, 3, 4, 5, 6, 7, 7, 1, 2, 3, 4, 5, 6, 7, 8,
							 1, 2, 3, 4, 5, 6, 7, 7, 1, 2, 3, 4, 5, 6, 7, 8,};
	misc.createBinary(src, bytes);

	uint32_t length = 0x10;
	uint32_t needle_ln = 8;
	uint64_t needle_idx = 0x38;
	uint64_t expected_idx = 0x30; // sanitized
	vector<uint8_t> needle(bytes.begin() + needle_idx, bytes.begin() + needle_idx + needle_ln);
	stringstream needle_ss;
	for ( uint32_t i = 0; i < needle_ln; i++ )
	{
		needle_ss << hex << setw(2) << setfill('0') << +needle[i];
	}
	const vector<string> argv = {src, "-fh " + needle_ss.str(), "-l " + to_string(length), "-b"};

	vector<string> expected_lines = getExpectedLines(name, bytes, TRIPLE, expected_idx, length);

	cout << "expected_lines" << endl;
	for ( const string& s : expected_lines )
		cout << s << endl;

	callApp(argv, expected_lines);

	remove(src.c_str());
}

TEST_F(HexterTest, testDelete)
{
	uint64_t binary_size = 0x60;
	uint32_t start = 0x10;
	uint32_t length = 0x10;

	string name = "testDelete.bla";
	string src = temp_dir + "/" + name;
	vector<uint8_t> bytes = misc.createBinary(src, binary_size);
	vector<uint8_t> deleted_bytes = bytes;
	deleted_bytes.erase(deleted_bytes.begin() + start, deleted_bytes.begin() + start + length);

	const vector<string> argv = {src, "-d ", "-s " + to_string(start), "-l " + to_string(length), "-x"};

	vector<string> expected_lines = getExpectedLines(name, deleted_bytes, HEX, start, DEFAULT_LENGTH);

//	cout << "bytes"<<endl;
//	for ( uint8_t b : bytes )
//		cout << hex<<+b << " ";
//	cout << endl;
//	cout << "expected_lines"<<endl;
//	for ( const string& s : expected_lines )
//		cout << s << endl;

	callApp(argv, expected_lines);

	remove(src.c_str());
}

#endif
