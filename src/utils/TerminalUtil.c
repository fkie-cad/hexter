#include <stdio.h>

#include "TerminalUtil.h"

void initTermios(int echo, struct termios* old)
{
    struct termios new_t;

    tcgetattr(0, old); /* grab old terminal i/o settings */
    new_t = *old; /* make new_t settings same as old settings */
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
void resetTermios(struct termios* old)
{
    tcsetattr(0, TCSANOW, old);
}

/**
 * Read 1 character
 * - echo defines echo mode
 */
char getch_(int echo)
{
    struct termios old_t;
    char ch;
    initTermios(echo, &old_t);
    ch = getchar();
    resetTermios(&old_t);
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
 * Read 1 character without echo.
 */
char _getch()
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
