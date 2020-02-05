#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "Converter.h"

int parseUint64Auto(const char* arg, uint64_t* value)
{
//	uint8_t base = 10;
//	if ( arg[0] != 0 && arg[1] != 0 && arg[0] ==  '0' && arg[1] ==  'x')
//		base = 16;

	return parseUint64(arg, value, 0);
}

int parseUint64(const char* arg, uint64_t* value, uint8_t base)
{
	char* endptr;
	int err_no = 0;
	errno = 0;
	uint64_t result;

	if ( base != 10 && base != 16 && base != 0 )
	{
		fprintf(stderr, "Error: Unsupported base %u!\n", base);
		return 1;
	}

	if ( arg[0] ==  '-' )
	{
		fprintf(stderr, "Error: %s could not be converted to a number: is negative!\n", arg);
		return 2;
	}

#if defined(_WIN32)
	result = strtoull(arg, &endptr, base);
#else
	result = strtoul(arg, &endptr, base);
#endif
	err_no = errno;

	if ( endptr == arg )
	{
		fprintf(stderr, "Error: %s could not be converted to a number: Not a number!\n", arg);
		return 3;
	}
	if ( result == UINT64_MAX && err_no == ERANGE )
	{
		fprintf(stderr, "Error: %s could not be converted to a number: Out of range!\n", arg);
		return 4;
	}

	*value = result;
	return 0;
}

int parseUint32Auto(const char* arg, uint32_t* value)
{
	uint64_t result;
	int s = parseUint64Auto(arg, &result);
	if ( s != 0 ) return s;
	if ( result > UINT32_MAX )
	{
		fprintf(stderr, "Error: %s could not be converted to a 4 byte int: Out of range!\n", arg);
		return 5;
	}

	*value = (uint32_t) result;
	return 0;
}

int parseUint32(const char* arg, uint32_t* value, uint8_t base)
{
	uint64_t result;
	int s = parseUint64(arg, &result, base);
	if ( s != 0 ) return s;
	if ( s > UINT32_MAX )
	{
		fprintf(stderr, "Error: %s could not be converted to a 4 byte int: Out of range!\n", arg);
		return 5;
	}

	*value = (uint32_t) result;
	return 0;
}

int parseUint16Auto(const char* arg, uint16_t* value)
{
	uint64_t result;
	int s = parseUint64Auto(arg, &result);
	if ( s != 0 ) return s;
	if ( result > UINT16_MAX )
	{
		fprintf(stderr, "Error: %s could not be converted to a 2 byte int: Out of range!\n", arg);
		return 5;
	}

	*value = (uint16_t) result;
	return 0;
}

int parseUint16(const char* arg, uint16_t* value, uint8_t base)
{
	uint64_t result;
	int s = parseUint64(arg, &result, base);
	if ( s != 0 ) return s;
	if ( s > UINT16_MAX )
	{
		fprintf(stderr, "Error: %s could not be converted to a 2 byte int: Out of range!\n", arg);
		return 5;
	}

	*value = (uint16_t) result;
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

/**
 * Parse arg as uint8 into value
 *
 * @param	arg const char* the raw arg
 * @param	value uint8_t* the parsed value container
 * @param	base uint8_t the base of the number conversion
 */
int parseUint8(const char* arg, uint8_t* value, uint8_t base)
{
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

uint16_t swapUint16(uint16_t value)
{
	return (((value & 0x00FF) << 8) |
			((value & 0xFF00) >> 8));
}

uint32_t swapUint32(uint32_t value)
{
	return (((value & 0x000000FF) << 24) |
			((value & 0x0000FF00) <<  8) |
			((value & 0x00FF0000) >>  8) |
			((value & 0xFF000000) >> 24));
}

uint64_t swapUint64(uint64_t value)
{
	return (((value & 0x00000000000000FF) << 56) |
			((value & 0x000000000000FF00) << 40) |
			((value & 0x0000000000FF0000) << 24) |
			((value & 0x00000000FF000000) <<  8) |
			((value & 0x000000FF00000000) >>  8) |
			((value & 0x0000FF0000000000) >> 24) |
			((value & 0x00FF000000000000) >> 40) |
			((value & 0xFF00000000000000) >> 56));
}

void formatTimeStampD(time_t t, char* res, size_t res_size)
{
	static const char format[] = "%a %d %b %Y";

	formatTimeStamp(t, res, res_size, format);
}

/**
 * Format a given timestamp.
 * The format is a string like "%a %d %b %Y".
 * a: short weekday, d: day, b: short month, y: short Year
 * A: long weekday, Y: full year, B: full month
 *
 * @param t
 * @param res
 * @param res_size
 * @param format
 */
int formatTimeStamp(time_t t, char* res, size_t res_size, const char* format)
{
	struct tm* ts;
	ts = localtime(&t);

	if ( strftime(res, res_size, format, ts) == 0 )
	{
//		printf( "strftime(3): cannot format supplied date/time into buffer of size %lu using: '%s'\n",
//					res_size, format);
		return -1;
	}
	
	return 0;
}