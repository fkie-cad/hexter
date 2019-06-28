#include <stdio.h>
#include <termios.h>

#include "TerminalUtil.h"

struct termios old, new_t;

void initTermios(int echo)
{
	tcgetattr(0, &old); /* grab old terminal i/o settings */
	new_t = old; /* make new_t settings same as old settings */
	new_t.c_lflag &= ~ICANON; /* disable buffered i/o */
	if (echo) {
		new_t.c_lflag |= ECHO; /* set echo mode */
	} else {
		new_t.c_lflag &= ~ECHO; /* set no echo mode */
	}
	tcsetattr(0, TCSANOW, &new_t); /* use these new terminal i/o settings now */
}

/**
 * Restore old terminal i/o settings
 */
void resetTermios()
{
	tcsetattr(0, TCSANOW, &old);
}

/**
 * Read 1 character
 * - echo defines echo mode
 */
char getch_(int echo)
{
	char ch;
	initTermios(echo);
	ch = getchar();
	resetTermios();
	return ch;
}

/**
 * Read 1 character without echo.
 */
char getch()
{
	return getch_(0);
}

/**
 * Read 1 character with echo.
 */
char getche()
{
	return getch_(1);
}
