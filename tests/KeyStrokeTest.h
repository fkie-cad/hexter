#ifndef G_TESTS_KEY_STROKE_TEST_H
#define G_TESTS_KEY_STROKE_TEST_H

#include <termios.h>
#include <stdio.h>

#include <iostream>

#include "../src/utils/TerminalUtil.c"

using namespace std;

class KeyStrokeTest : public testing::Test
{
	protected:


	public:
};


TEST_F(KeyStrokeTest, testFGetC)
{
	char c;
	printf("(getche example) please type a letter: ");
	c = getche();
	printf("\nYou typed: %c\n", c);
	printf("(getch example) please type a letter...");
	c = getch();
	printf("\nYou typed: %c\n", c);
}

#endif
