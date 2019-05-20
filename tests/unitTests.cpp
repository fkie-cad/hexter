#include <gtest/gtest.h>

#include "utils/ConverterTest.h"
#include "FinderTest.h"
#include "PayloaderTest.h"
#include "HexterTest.h"

uint64_t file_size;
char file_name[PATH_MAX];
uint64_t start;
uint64_t length;
uint8_t ascii_only;
uint8_t hex_only;
uint8_t clean_printing;
uint8_t find_f;
uint8_t insert_f;
uint8_t overwrite_f;

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	int ret = RUN_ALL_TESTS();
	return ret;
}