#ifndef HEXTER_SRC_UTILS_TERINAL_UTIL_H
#define HEXTER_SRC_UTILS_TERINAL_UTIL_H

/**
 * Linux implementation of gech().
 * Immediately get an inputted char without waiting for ENTER.
 */

void initTermios(int echo);
void resetTermios();
char getch_(int echo);
char getch();
char _getch();
char getche();

#endif
