#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Converter.h"

int parseUint64Auto(const char* arg, uint64_t* value)
{
	uint8_t base = 10;
	if ( arg[0] != 0 && arg[1] != 0 && arg[0] ==  '0' && arg[1] ==  'x')
		base = 16;

	return parseUint64(arg, value, base);
}

int parseUint64(const char* arg, uint64_t* value, uint8_t base)
{
	char* endptr;
	int err_no = 0;
	errno = 0;
	uint64_t result;

	if ( base != 10 && base != 16 )
	{
		fprintf(stderr, "Error: Unsupported base %u!\n", base);
		return 1;
	}

	if ( arg[0] ==  '-' )
	{
		fprintf(stderr, "Error: %s could not be converted to a number: is negative!\n", arg);
		return 2;
	}

	result = strtoul(arg, &endptr, base);
	err_no = errno;

	if ( endptr == arg )
	{
		fprintf(stderr, "Error: %s could not be converted to a number: Not a number!\n", arg);
		return 3;
	}
	if ( result == UINT64_MAX && err_no == ERANGE)
	{
		fprintf(stderr, "Error: %s could not be converted to a number: Out of range!\n", arg);
		return 4;
	}

	*value = result;
	return 0;
}

int parseUint8Auto(const char* arg, uint8_t* value)
{
	uint64_t result;
	int s = parseUint64Auto(arg, &result);
	if ( s != 0 ) return s;
	if ( result > UINT8_MAX )
	{
		fprintf(stderr, "Error: %s could not be converted to a byte: Out of range!\n", arg);
		return 5;
	}

	*value = (uint8_t) result;
	return 0;
}

int parseUint8(const char* arg, uint8_t* value, uint8_t base)
{
//	char* endptr;
//	int err_no = 0;
//	errno = 0;
//	uint8_t base = 16;
//	uint8_t result;
//
//	if ( strlen(arg) != 2 )
//	{
//		fprintf(stderr, "Error: %s is not a byte!\n", arg);
//		return 1;
//	}
//	if ( !isHexChar(arg[0]) || !isHexChar(arg[1]) )
//	{
//		fprintf(stderr, "Error: %s is not in hex format!\n", arg);
//		return 2;
//	}
//
//	result = strtoul(arg, &endptr, base);
//	err_no = errno;
//
//	if ( endptr == arg )
//	{
//		fprintf(stderr, "Error: %s could not be converted to a number: Not a number!\n", arg);
//		return 3;
//	}
//	if ( result == UINT64_MAX && err_no == ERANGE)
//	{
//		fprintf(stderr, "Error: %s could not be converted to a number: Out of range!\n", arg);
//		return 4;
//	}

	uint64_t result;
	int s = parseUint64(arg, &result, base);
	if ( s != 0 ) return s;
	if ( s > UINT8_MAX )
	{
		fprintf(stderr, "Error: %s could not be converted to a byte: Out of range!\n", arg);
		return 5;
	}

	*value = (uint8_t) result;
	return 0;
}

uint8_t isHexChar(const char c)
{
	return (48 <= c && c <= 57) // 0-9
		|| (65 <= c && c <= 70) // A-F
		|| (97 <= c && c <= 102); // a-f
}