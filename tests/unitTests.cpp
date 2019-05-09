#include <gtest/gtest.h>

#include "utils/ConverterTest.h"
#include "HexterTest.h"
#include "PayloaderTest.h"

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	int ret = RUN_ALL_TESTS();
	return ret;
}