#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__linux__) || defined(__linux) || defined(linux)
	#include "TerminalUtil.h"
#endif

#include "Helper.h"
#include "../Globals.h"

#if defined(__linux__) || defined(__linux) || defined(linux)
	#define PATH_SEPARATOR 0x2F
#elif defined(_WIN32)
	#define PATH_SEPARATOR 0x5C
#endif

/**
 * Expand a leading '~' in src to a full file path in dest.
 *
 * @param src char*
 * @param dest char*
 */
void expandFilePath(const char* src, char* dest)
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
 * Create a temporary file and store its name in buf.
 *
 * @param	buf char[128]
 * @param	prefix char* name prefix of the tmp file
 * @return	int status code
 */
int getTempFile(char* buf, const char* prefix)
{
	int s = 1;
#if defined(__linux__) || defined(__linux) || defined(linux)
	snprintf(buf, 128, "/tmp/%sXXXXXX.tmp", prefix);
	buf[127] = 0;

	s = mkstemps(buf, 4);
#endif
	return s;
}

/**
 * Extract the file_name out of a file_path.
 * "Light" version just pointing to the file_name in the memory of file_path.
 *
 * @param file_path char*
 * @param file_name char**
 */
void getFileNameL(char* file_path, char** file_name)
{
	if ( strnlen(file_path, PATH_MAX) == 0 ) return;

	int64_t offset = getFileNameOffset(file_path);
	*file_name = &file_path[offset];
}

/**
 * Extract the file_name out of a file_path.
 * Copying the found name into file_name.
 * Make sure, char[] has a capacity of PATH_MAX!
 *
 * @param file_path char*
 * @param file_name char*
 */
void getFileName(const char* file_path, char* file_name)
{
	int64_t offset;
	size_t file_name_ln;
	size_t file_path_ln = strnlen(file_path, PATH_MAX);

	if ( file_path_ln == 0 ) return;

	offset = getFileNameOffset(file_path);
	file_name_ln = file_path_ln - offset;
	memcpy(file_name, &file_path[offset], file_name_ln);
	file_name[file_name_ln] = 0;
}

/**
 * Extract the file_name out of a file_path.
 * Copying the found name into file_name allocated char*.
 * Caller is responsible for freeing it!
 *
 * @param 	file_path char*
 * @return	char* the file name
 */
char* getFileNameP(const char* file_path)
{
	int64_t offset;
	size_t file_name_ln;
	char* file_name;
	size_t file_path_ln = strnlen(file_path, PATH_MAX);

	if ( file_path_ln == 0 ) return NULL;

	offset = getFileNameOffset(file_path);
	file_name_ln = file_path_ln - offset;
	file_name = (char*) calloc(file_name_ln+1, sizeof(char));
	memcpy(file_name, &file_path[offset], file_name_ln);

	return file_name;
}

/**
 * Extract the file_name out of a file_path.
 *
 * @param file_path char*
 * @param file_name char*
 */
int64_t getFileNameOffset(const char* file_path)
{
	size_t file_path_ln = strnlen(file_path, PATH_MAX);
	int64_t i = 0;
	for ( i = file_path_ln-1; i >= 0; i--)
	{
		if ( file_path[i] == PATH_SEPARATOR )
			break;
	}
	return ( i >= 0 ) ? i+1 : 0;
}

/**
 * Count the width (string length) of a hex value representation of an uint.
 *
 * @param	value uint64_t the value
 * @return	uint8_t the width
 */
uint8_t countHexWidth64(uint64_t value)
{
	uint8_t width = 16;
	uint8_t t8;
	uint16_t t16;
	uint32_t t32 = (uint32_t) (value >> 32u);
	if ( t32 == 0 )
	{
		width -= 8;
		t32 = (uint32_t) value;
	}
	t16 = (uint16_t) (t32 >> 16u);
	if ( t16 == 0 )
	{
		width -= 4;
		t16 = (uint16_t) t32;
	}
	t8 = (uint8_t) (t16 >> 8u);
	if ( t8 == 0 )
	{
		width -= 2;
	}
	return width;
}

/**
 * Normalize a match a colsize value and fill the remainder.
 *
 * @param	offset uint64_t the offset
 * @param	remainder uint8_t the remainder
 * @return
 */
uint64_t normalizeOffset(uint64_t offset, uint8_t* remainder)
{
	uint8_t col_size = getColSize();
	*remainder = (offset % col_size);

	offset -= *remainder;

	return offset;
}

/**
 * Get the colsize depending on the selected printing method.
 *
 * @return	uint8_t the col size
 */
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

/**
 * Split string into its parts, determined by the delimiter.
 * str will be changed!!!
 * TODO: show, if max_bucket is exceeded
 *
 * @param	str char* the source string
 * @param	delimiter char* the splitting delimiter
 * @param	bucket char** the bucket to hold the parts (should be allocated memory)
 * @param	bucket_max size_t the max size of the bucket (if there are more parts, the loop will break before)
 * @return	size_t the size of the actual bucket, i.e. number of found parts.
 */
size_t split(char* str, const char* delimiter, char** bucket, const size_t bucket_max)
{
	char *token;

	token = strtok(str, delimiter);
	size_t token_id = 0;

	while ( token != NULL )
	{
		bucket[token_id] = token;
		token = strtok(NULL, delimiter);

		token_id++;
		if ( token_id >= bucket_max )
			break;
	}

	return token_id;
}

bool confirmContinueWithNextRegion(char* name, uint64_t address)
{
	char input;
	int counter = 0;

	printf("\n");
	printf("Continue with next region");
	if ( name != NULL ) printf(": %s ", name);
	printf(" (0x%p) (c/q)?\n", address);

	while ( 1 )
	{
#if defined(_WIN32)
		input = _getch();
#else
		input = getch();
#endif
		if ( input == 'c' )
			return true;
		else if ( input == 'q' )
			return false;
		else if ( counter > 100 )
			return false;

		counter++;
	}
//	return false;
}

void setAnsiFormat(char* format)
{
	printf("%s", format);
}

void resetAnsiFormat()
{
	printf("\033[0m");
}
