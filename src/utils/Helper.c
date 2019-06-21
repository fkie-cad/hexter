#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "Helper.h"
#include "../Globals.h"

void expandFilePath(char* src, char* dest)
{
	const char* env_home;
	if ( src[0] == '~' )
	{
		env_home = getenv("HOME");
		if ( env_home != NULL )
		{
			snprintf(dest, PATH_MAX, "%s/%s", env_home, &src[2]);
		}
		else
		{
			snprintf(dest, PATH_MAX, "%s", src);
		}
	}
	else
	{
		snprintf(dest, PATH_MAX, "%s", src);
	}
	dest[PATH_MAX-1] = 0;
}

/**
 *
 * @param buf char[128]
 * @param prefix
 * @return
 */
int getTempFile(char* buf, char* prefix)
{
	int s = 1;
#if defined(__linux__) || defined(__linux) || defined(linux)
	snprintf(buf, 128, "/tmp/%sXXXXXX.tmp", prefix);
	buf[127] = 0;

	s = mkstemps(buf, 4);
#endif
	return s;
}

uint8_t countHexWidth64(uint64_t value)
{
	uint8_t width = 16;
	uint8_t t8;
	uint16_t t16;
	uint32_t t32 = (uint32_t) (value >> 32);
	if ( t32 == 0 )
	{
		width -= 8;
		t32 = (uint32_t) value;
	}
	t16 = (uint16_t) (t32 >> 16);
	if ( t16 == 0 )
	{
		width -= 4;
		t16 = (uint16_t) t32;
	}
	t8 = (uint8_t) (t16 >> 8);
	if ( t8 == 0 )
	{
		width -= 2;
	}
	return width;
}

uint64_t normalizeOffset(uint64_t offset, uint8_t* remainder)
{
	uint8_t col_size = getColSize();
	*remainder = (offset % col_size);

	offset -= *remainder;

	return offset;
}

uint8_t getColSize()
{
	uint8_t col_size = 0;
	if ( print_col_mask == (print_offset_mask | print_ascii_mask | print_hex_mask))
		col_size = TRIPLE_COL_SIZE;
	else if ( print_col_mask == (print_ascii_mask | print_hex_mask))
		col_size = DOUBLE_COL_SIZE;
	else if ( print_col_mask == print_ascii_mask )
		col_size = ASCII_COL_SIZE;
	else if ( print_col_mask == print_hex_mask )
		col_size = HEX_COL_SIZE;
	return col_size;
}