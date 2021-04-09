#ifndef HEXTER_SRC_UTILS_TERINAL_UTIL_H
#define HEXTER_SRC_UTILS_TERINAL_UTIL_H

/**
 * Linux implementation of getch().
 * Immediately get an inputted char without waiting for ENTER.
 */

#include <termios.h>

void initTermios(int echo, struct termios* old);
void resetTermios(struct termios* old);
char getch_(int echo);
char getch();
char _getch();
char getche();

#endif
