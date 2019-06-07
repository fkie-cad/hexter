#ifndef HEXTER_SRC_UTILS_TERINAL_UTIL_H
#define HEXTER_SRC_UTILS_TERINAL_UTIL_H

void initTermios(int echo);
void resetTermios();
char getch_(int echo);
char getch();
char getche();

#endif
