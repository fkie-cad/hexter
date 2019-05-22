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

#include "../src/Globals.h"
//#include "../src/hexter.c"
#define BLOCKSIZE_LARGE 0x400

using namespace std;

class HexterTest : public testing::Test
{
	protected:
		const string prog_dir = "./";
		const string prog = "hexter";
		static string temp_dir;
		const string vs = "1.3.0";

		std::random_device rd;
		static mt19937_64* gen;
		static uniform_int_distribution<uint8_t>* dis;

		enum PrintType { HEX, ASCII, DOUBLE };

		const vector<string> missing_args_lines = {
				"Usage: ./"+prog+" filename [options]",
				"Usage: ./"+prog+" [options] filename",
				"Version: "+vs,
				"",
		};

		const vector<string> help_lines = {
				"Usage: ./"+prog+" filename [options]",
				"Usage: ./"+prog+" [options] filename",
				"Version: "+vs,
				" * -s:uint64_t Startoffset. Default = 0.",
				" * -l:uint64_t Length of the part to display. Default = 50.",
				" * -a ASCII only print.",
				" * -x HEX only print.",
				" * -c Clean output (no text formating in the console).",
				" * -i Insert hex byte sequence (destructive!).",
				" * -o Overwrite hex byte sequence (destructive!).",
				" * -f Find hex byte sequence.",
				" * -h Print this.",
				"",
				"Example: "+prog_dir+prog+" path/to/a.file -s 100 -l 128 -x",
				"Example: "+prog_dir+prog+" path/to/a.file -i dead -s 0x100",
				"Example: "+prog_dir+prog+" path/to/a.file -o 0bea -s 0x100",
				"Example: "+prog_dir+prog+" path/to/a.file -f f001 -s 0x100",
				""
		};

		const vector<string> unknown_args_lines = {
				"INFO: Unknown arg type \"-x0\"",
				"File a does not exist.",
				""
		};

		const vector<string> not_passed_value_args_lines = {
				"INFO: Arg \"-s\" has no value! Skipped!",
				"File a does not exist.",
				""
		};

		const vector<string> incompatible_args_lines = {
				"ERROR: overwrite, insert and find have to be used exclusively!",
				""
		};

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

		string createCommand(const string& args) const
		{
			stringstream ss;
			ss << prog_dir << prog << " "<<args.substr(0,500)<<" 2>&1";

			return ss.str();
		}

		int openFile(const string& command, FILE *&fi) const
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

			while (fgets(raw_line, sizeof(raw_line), fi) != NULL)
			{
				if ( strlen(raw_line) )
				{
					string line = string(raw_line);
					line = line.substr(0, line.size()-1);
					lines.emplace_back(line);
				}
			}

			return lines;
		}

		void callApp(const vector<string>& argv, const vector<string>& expected_lines)
		{
			string args = "";
			for ( string a : argv ) args = args.append(a + " ");
			string command = createCommand(args);
			FILE* fi = nullptr;
			openFile(command, fi);
			vector<string> lines = getResult(fi);

			lines.resize(expected_lines.size());

			cout << "lines"<<endl;
			for ( string s : lines )
				cout << s << endl;

			fclose(fi);
			EXPECT_EQ(lines, expected_lines);
		}

		vector<string> getExpectedLines(vector<uint8_t> bytes, PrintType type, uint64_t start, uint64_t length);
		void getDoubleCols(vector<uint8_t>& block, vector<string>& lines);
		void getHexCols(vector<uint8_t>& block, vector<string>& lines);
		void getAsciiCols(vector<uint8_t>& block, vector<string>& lines, uint8_t col_size);
		uint64_t fillHexCol(uint64_t i, vector<uint8_t>& block, stringstream& ss);
		void fillAsciiCol(uint64_t i, vector<uint8_t>& block, stringstream& ss, uint8_t col_size) const;

	public:

		static void SetUpTestCase()
		{
			std::random_device rd;
			gen = new mt19937_64(rd());
			dis = new uniform_int_distribution<uint8_t>(0, UINT8_MAX);

			temp_dir = getTempDir("HexterTest");
		}

		static void TearDownTestCase()
		{
			delete(gen);
			delete(dis);

			rmdir(temp_dir.c_str());
		}
};
mt19937_64* HexterTest::gen = nullptr;
uniform_int_distribution<uint8_t>* HexterTest::dis = nullptr;
string HexterTest::temp_dir;

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
	const vector<string> argv_fo = {"a -f 10 -o 10"};
	const vector<string> argv_fi = {"a -f 10 -i 10"};
	const vector<string> argv_io = {"a -i 10 -o 10"};

	callApp(argv_fo, incompatible_args_lines);
	callApp(argv_fi, incompatible_args_lines);
	callApp(argv_io, incompatible_args_lines);
}

TEST_F(HexterTest, testMainWithHelpArg)
{
	const vector<string> argv = {"-h a"};

	callApp(argv, help_lines);
}

TEST_F(HexterTest, testMainWithNotExistingFile)
{
	string src = "not/ex/ist.ing";
	const vector<string> argv = {src};

	callApp(argv, {"File "+src+" does not exist."});
}

TEST_F(HexterTest, testMainWithRandomFile)
{
	uint64_t binary_size = 128;
	string src = temp_dir+"/testMainWithRandomFile.bind";
	vector<uint8_t> bytes = createBinary(src, binary_size);
	const vector<string> argv = {src};

	vector<string> expected_lines = getExpectedLines(bytes, DOUBLE, 0, 0x50);

	callApp(argv, expected_lines);

	remove(src.c_str());
}

TEST_F(HexterTest, testMainWithRandomFileNegativeArgs)
{
	uint64_t binary_size = 128;
	string src = temp_dir+"/testMainWithRandomFile.bind";
	vector<uint8_t> bytes = createBinary(src, binary_size);
	const vector<string> argv = {src, "-s -10", "-l -100"};

	vector<string> expected_lines = {"Error: -10 could not be converted to a number: is negative!"};
	callApp(argv, expected_lines);

	const vector<string> argv2 = {src, "-s 10", "-l -100"};
	vector<string> expected_lines2 = {"Error: -100 could not be converted to a number: is negative!"};
	callApp(argv2, expected_lines2);

	const vector<string> argv3 = {src, "-s dd", "-l -100"};
	vector<string> expected_lines3 = {"Error: dd could not be converted to a number: Not a number!"};
	callApp(argv3, expected_lines3);

	const vector<string> argv4 = {src, "-s 10", "-l ff"};
	vector<string> expected_lines4 = {"Error: ff could not be converted to a number: Not a number!"};
	callApp(argv4, expected_lines4);

	remove(src.c_str());
}

TEST_F(HexterTest, testMainWithRandomFileCustomParams)
{
	uint64_t binary_size = 0x200;
	string src = temp_dir+"/testMainWithRandomFileCustomParams.bind";
	vector<uint8_t> bytes = createBinary(src, binary_size);
	const vector<string> argv = {src, "-s 0x10", "-l 0x100"};

	vector<string> expected_lines = getExpectedLines(bytes, DOUBLE, 0x10, 0x100);

	callApp(argv, expected_lines);

	remove(src.c_str());
}

TEST_F(HexterTest, testMainWithRandomFileHexOnlyPrint)
{
	uint64_t binary_size = 0x100;
	string src = temp_dir+"/testMainWithRandomFileHex.bind";
	vector<uint8_t> bytes = createBinary(src, binary_size);
	const vector<string> argv = {src, "-x"};

	vector<string> expected_lines = getExpectedLines(bytes, HEX, 0x0, 0x50);

	callApp(argv, expected_lines);

	remove(src.c_str());
}

TEST_F(HexterTest, testMainWithRandomFileAsciiOnlyPrint)
{
	uint64_t binary_size = 0x100;
	string src = temp_dir+"/testMainWithRandomFileAscii.bind";
	vector<uint8_t> bytes = createBinary(src, binary_size);
	const vector<string> argv = {src, "-a"};

	vector<string> expected_lines = getExpectedLines(bytes, ASCII, 0x0, 0x50);

	callApp(argv, expected_lines);

	remove(src.c_str());
}

vector<string> HexterTest::getExpectedLines(vector<uint8_t> bytes, PrintType type, uint64_t start, uint64_t length)
{
	uint64_t end = start + length;
	uint16_t block_size = BLOCKSIZE_LARGE;
	uint64_t block_start = start;
	uint64_t read_size = 0;
	uint64_t parts = length / block_size;
	if ( length % block_size != 0 ) parts++;

	vector<string> lines;

	for ( uint64_t p = 0; p < parts; p++ )
	{
		read_size = block_size;
		if ( block_start + read_size > end ) read_size = end - block_start;

		vector<uint8_t> block(&bytes[block_start], &bytes[block_start+read_size]);

		if ( type == HEX )
			getHexCols(block, lines);
		else if ( type == ASCII )
			getAsciiCols(block, lines, ASCII_COL_SIZE);
		else if ( type == DOUBLE )
			getDoubleCols(block, lines);

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
		if ( temp_i >= block.size() )
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
		if ( temp_i >= block.size() )
			break;

		ss.width(2);
		ss.fill('0');
		ss << hex<<uppercase << +block[temp_i] << " ";
	}

	return k;
}

TEST_F(HexterTest, testOverwrite)
{
	uint64_t binary_size = 0x50;
	string src = temp_dir+"/testOverwrite.bla";
	vector<uint8_t> bytes = createBinary(src, binary_size);
	const vector<string> argv = {src, "-o 0000dead0bea0000", "-x"};
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

	vector<string> expected_lines = getExpectedLines(bytes, HEX, start, length);

	cout << "expected_lines"<<endl;
	for ( string s : expected_lines )
		cout << s << endl;

	callApp(argv, expected_lines);

	remove(src.c_str());
}

TEST_F(HexterTest, testInsert)
{
	uint64_t binary_size = 0x60;
	string src = temp_dir+"/testInsert.bla";
	vector<uint8_t> bytes = createBinary(src, binary_size);
	uint32_t start = 0x0;
	uint32_t length = 0x50;
	unsigned char pl[] = {
			0,0,222,173,11,234,0,0
	};
	uint32_t payload_ln = 8;
	vector<uint8_t> payloaded_bytes(bytes.begin(), bytes.end());
	stringstream pl_ss;
	for ( int i = 0; i < payload_ln; i++ )
	{
		payloaded_bytes.insert(payloaded_bytes.begin() + (start + i), pl[i]);
		pl_ss << hex << setw(2) << setfill('0') << +pl[i];
	}
	const vector<string> argv = {src, "-i "+pl_ss.str(), "-s "+to_string(start), "-x"};
	for ( string arg : argv ) cout << arg << endl;

	vector<string> expected_lines = getExpectedLines(payloaded_bytes, HEX, start, length);

	cout << "expected_lines"<<endl;
	for ( string s : expected_lines )
		cout << s << endl;

	callApp(argv, expected_lines);

	remove(src.c_str());
}

TEST_F(HexterTest, testFind)
{
	uint64_t binary_size = 0x60;
	string src = temp_dir+"/testInsert.bla";
	vector<uint8_t> bytes = createBinary(src, binary_size);
	uint32_t start = 0x0;
	uint32_t length = 0x50;
	unsigned char pl[] = {
			0,0,222,173,11,234,0,0
	};
	uint32_t payload_ln = 8;
	vector<uint8_t> payloaded_bytes(bytes.begin(), bytes.end());
	stringstream pl_ss;
	for ( int i = 0; i < payload_ln; i++ )
	{
		payloaded_bytes.insert(payloaded_bytes.begin() + (start + i), pl[i]);
		pl_ss << hex << setw(2) << setfill('0') << +pl[i];
	}
	const vector<string> argv = {src, "-i "+pl_ss.str(), "-s "+to_string(start), "-x"};
	for ( string arg : argv ) cout << arg << endl;

	vector<string> expected_lines = getExpectedLines(payloaded_bytes, HEX, start, length);

	cout << "expected_lines"<<endl;
	for ( string s : expected_lines )
		cout << s << endl;

	callApp(argv, expected_lines);

	remove(src.c_str());
}

#endif
