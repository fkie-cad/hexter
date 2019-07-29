#include <stdio.h>
#include "../src/hexter.h"

int main()
{
//	int s = printFile("hexter.exe", 0, 0x100);
	int s = printString("blah");
	printf("s: %d\n", s);
	return 0;
}