#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__linux__) || defined(__linux) || defined(linux)
	#include <dirent.h>
	#include "TerminalUtil.h"
#endif
#if defined(_WIN32)
	#include <conio.h>
#endif

#include "Helper.h"
#include "../Globals.h"

#if defined(__linux__) || defined(__linux) || defined(linux) || defined(__APPLE__)
	#define PATH_SEPARATOR 0x2F
#elif defined(_WIN32)
	#define PATH_SEPARATOR 0x5C
#endif

/**
 * Expand a leading '~' in src to a full file path in dest.
 * dest has be size of PATH_MAX.
 *
 * @param src char*
 * @param dest char*
 */
void expandFilePath(const char* src, char* dest)
{
	const char* env_home;
	
	if ( strlen(src) == 0 ) return;

#if defined(__linux__) || defined(__linux) || defined(linux) || defined(__APPLE__)
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
#endif
	{
		snprintf(dest, PATH_MAX, "%s", src);
	}
	dest[PATH_MAX-1] = 0;
}

/**
 * Create a temporary file and store its name in buf.
 * Currently Linux only.
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
#else
	(void)buf;
	(void)prefix;
#endif
	return s;
}

/**
 * Extract the base file name out of a file_path.
 * "Light" version just pointing to the file_name in the memory of file_path.
 *
 * @param file_path char*
 * @param file_name char**
 */
void getFileNameL(char* path, char** file_name)
{
	if ( strnlen(path, PATH_MAX) == 0 ) return;

	int64_t offset = getFileNameOffset(path);
	*file_name = &path[offset];
}

/**
 * Extract the base file name out of a file_path.
 * Copying the found name into file_name.
 * Make sure, file_name char[] has a capacity of PATH_MAX!
 *
 * @param path char*
 * @param name char*
 */
void getFileName(const char* path, char* name)
{
	int32_t offset;
	size_t file_name_ln;
	int32_t file_path_ln = (int32_t)strnlen(path, PATH_MAX);

	if ( file_path_ln == 0 )
	{
		name[0] = 0;
		return;
	}

	offset = getFileNameOffset(path);
	file_name_ln = file_path_ln - offset;
	if ( file_path_ln < offset ) file_name_ln = 0;
	memcpy(name, &path[offset], file_name_ln);
	name[file_name_ln] = 0;
}

/**
 * Extract the base file name out of a file_path.
 * Copying the found name into file_name allocated char*.
 * Caller is responsible for freeing it!
 *
 * @param 	file_path char*
 * @return	char* the file name
 */
char* getFileNameP(const char* path)
{
	int32_t offset;
	size_t file_name_ln;
	char* file_name;
	uint32_t file_path_ln = (uint32_t)strnlen(path, PATH_MAX);

	if ( file_path_ln == 0 )
		return NULL;

	offset = getFileNameOffset(path);
	file_name_ln = file_path_ln - offset;
	file_name = (char*) calloc(file_name_ln+1, sizeof(char));
	if ( file_name == NULL )
		return NULL;
	memcpy(file_name, &path[offset], file_name_ln);

	return file_name;
}

/**
 * Extract the base file name offset out of a file_path.
 *
 * @param file_path char*
 * @param file_name char*
 */
int32_t getFileNameOffset(const char* path)
{
	size_t file_path_ln = strnlen(path, PATH_MAX);
	int64_t i = 0;
	for ( i = file_path_ln-1; i >= 0; i--)
	{
		if ( path[i] == PATH_SEPARATOR )
			break;
	}
	return ( i >= 0 ) ? (int32_t)(i+1) : 0;
}

/**
 * List all files in a directory.
 *
 * @param path char* the directory path.
 */
void listFilesOfDir(char* path)
{
#if defined(__linux__) || defined(__linux) || defined(linux)
	DIR *d;
	struct dirent *dir;
	d = opendir(path);

	if ( !d )
		perror("listFilesOfDir: could not open dir!\n");

	while ( (dir = readdir(d)) != NULL )
	{
		if ( dir->d_type == DT_REG )
			printf("%s, ", dir->d_name);
	}
	closedir(d);
	printf("\n");
#else
	(void)path;
#endif
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
 * Count the width (string length) of a hex value representation of an uint.
 *
 * @param	value uint32_t the value
 * @return	uint8_t the width
 */
uint8_t countHexWidth32(uint32_t value)
{
	uint8_t width = 8;
	uint8_t t8;
	uint16_t t16 = (uint16_t) (value >> 16u);
	if ( t16 == 0 )
	{
		width -= 4;
		t16 = (uint16_t) value;
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
 * @param	offset size_t the offset
 * @param	remainder uint8_t the remainder
 * @return
 */
size_t normalizeOffset(size_t offset, uint8_t* remainder)
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

Bool confirmContinueWithNextRegion(char* name, size_t address)
{
	char input;
	int counter = 0;

	printf("\n");
	printf("Continue with next region");
	if ( name != NULL ) printf(": %s ", name);
	printf(" (0x%p) (c/q)?\n", (void*)address);

	while ( 1 )
	{
		input = (char)_getch();
//#if defined(_WIN32)
//		input = _getch();
//#else
//		input = getch();
//#endif
		if ( input == CONTINUE )
			return true;
		else if ( input == QUIT )
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
