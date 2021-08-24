#ifndef TESTS_HEXTER_LIB_TEST
#define TESTS_HEXTER_LIB_TEST

#include <cstdio>
#include <cstdlib>
#include <cstdint>

//#include <filesystem>
#include <fstream>
#include <sstream>

#include <gtest/gtest.h>

#include "../src/hexter.h"

using namespace std;
//namespace fs = std::filesystem;

class HexterLibTest :public testing::Test
{
	protected:
		const char* pe_file = "tests/files/qappsrv.exe";
		const char* elf_file = "tests/files/hello_world_release.elf";

	public:

		static void SetUpTestCase()
		{
		}
};

TEST_F(HexterLibTest, test_hexter_printFile)
{
	size_t start = 0;
	size_t length = 0x100;

	hexter_printFile(pe_file, start, length);
}

TEST_F(HexterLibTest, test_hexter_printProcess)
{
	uint32_t _pid = 0;
	size_t _start = 0;
	size_t _length = 0x100;
	uint32_t flags = PROCESS_LIST_HEAPS | PROCESS_LIST_HEAP_BLOCKS | PROCESS_LIST_MEMORY | PROCESS_LIST_MODULES | PROCESS_LIST_RUNNING_PROCESSES;

	hexter_printProcess(_pid, _start, _length, flags);
}

#endif
