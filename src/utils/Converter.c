#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "Converter.h"

uint64_t parseUint64(char* argv)
{
	if ( argv[0] ==  '-' )
	{
		fprintf(stderr, "Error: %s could not be converted to a number: is negative!\n", argv);
		exit(0);
	}
	char* endptr;
	int err_no = 0;
	errno = 0;
	uint8_t base = 10;
	if ( argv[0] ==  '0' && argv[1] ==  'x')
		base = 16;

	uint64_t result = strtoul(argv, &endptr, base);
	err_no = errno;

	if (endptr == argv)
	{
		fprintf(stderr, "Error: %s could not be converted to a number: Not a number!\n", argv);
		exit(0);
	}
	if ( result == UINT64_MAX && err_no == ERANGE)
	{
		fprintf(stderr, "Error: %s could not be converted to a number: Out of range!\n", argv);
		exit(0);
	}

	return result;
}
