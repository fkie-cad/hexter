#ifndef G_TESTS_HEXTER_TEST_H
#define G_TESTS_HEXTER_TEST_H

#include "../../src/hexter.c"

class HexterTest : public testing::Test
{
	protected:

	public:

	static void SetUpTestCase()
	{
	}

};

TEST_F(HexterTest, testMainWithoutArgs)
{
	string arg = "";

//	callApp({arg}, missing_args_lines);
}

#endif
