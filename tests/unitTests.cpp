#include <gtest/gtest.h>

#include "../src/utils/common_fileio.c"
#include "../src/utils/Converter.c"
#include "../src/utils/TerminalUtil.c"
#include "../src/utils/Helper.c"
#include "../src/utils/Strings.c"
//#include "../src/Globals.h"
//#include "../src/Writer.h"
#include "../src/Finder.c"
#include "../src/Writer.c"

#include "utils/ConverterTest.h"
#include "utils/HelperTest.h"
#include "utils/StringsTest.h"
#include "FinderTest.h"
#include "KeyStrokeTest.h"
#include "WriterTest.h"
#include "HexterTest.h"

uint64_t file_size;
char file_path[PATH_MAX];
uint64_t length;
uint8_t print_col_mask;
uint8_t print_offset_mask;
uint8_t print_hex_mask;
uint8_t print_ascii_mask;
uint8_t clean_printing;
uint8_t find_f;
//uint8_t insert_f;
//uint8_t overwrite_f;
uint8_t continuous_f;

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	int ret = RUN_ALL_TESTS();
	return ret;
}
