#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Finder.h"
#include "Globals.h"
//#include "utils/Helper.h"
//#include "utils/common_fileio.h"
//#include "Printer.h"

static uint16_t* failure = NULL;

void Finder_initFailure(unsigned char* needle, uint32_t needle_ln)
{
//	int i = 0;
	failure = (uint16_t*) calloc(needle_ln, sizeof(uint16_t));
	computeFailure(needle, needle_ln, failure);
//	if ( failure != NULL )
//	{
//		printf("failure\n");
//		for ( i = 0; i < needle_ln; i++ )
//		{
//			printf("%u, ", failure[i]);
//		}
//		printf("\n");
//	}
//	else
//	{
//		printf("no failure initialized!\n");
//	}
}

/**
 * Find the needle in the file starting from an offset.
 *
 * @param	needle unsigned char*
 * @param	needle_ln uint32_t
 * @param	offset size_t
 * @return	size_t the offset of the found needle or FIND_FAILURE, if not found
 */
size_t find(const char* file_path, const unsigned char* needle, uint32_t needle_ln, size_t offset, size_t max_offset)
{
//	size_t n_found;

	uint16_t* failure;
	failure = (uint16_t*) calloc(needle_ln, sizeof(uint16_t));
//	failure = (uint16_t*) malloc(needle_ln*sizeof(uint16_t));
	computeFailure(needle, needle_ln, failure);

	size_t found = findNeedleInFile(file_path, needle, needle_ln, offset, max_offset);
//	uint8_t remainder = 0;

//	n_found = normalizeOffset(found, &remainder);
//	if ( found == FIND_FAILURE )
//		printf("Pattern not found!\n");
//	else
//		print(n_found, remainder);

	free(failure);

	return found;
}

/**
 * KMPMatch
 *
 * @param needle unsigned char*
 * @param needle_ln uint32_t
 * @param offset size_t
 * @return
 */
size_t findNeedleInFile(const char* file_path, const unsigned char* needle, uint32_t needle_ln, size_t offset, size_t max_offset)
{
//	printf("BLOCKSIZE_LARGE: %u\n",BLOCKSIZE_LARGE);
	FILE* fi;
	size_t found;

	fi = fopen(file_path, "rb");
	if ( !fi )
	{
		printf("ERROR: File %s does not exist.\n", file_path);
		return FIND_FAILURE;
	}

	found = findNeedleInFP(needle, needle_ln, offset, fi, max_offset);

	fclose(fi);

	return found;
}

/**
 * Find needle with given failure and FILE*.
 *
 * @param needle
 * @param needle_ln
 * @param offset
 * @param failure
 * @param fi
 * @return	size_t the found offset or FIND_FAILURE
 */
size_t findNeedleInFP(const unsigned char* needle, uint32_t needle_ln, size_t offset, FILE* fi, size_t max_offset)
{
	unsigned char buf[BLOCKSIZE_LARGE];
	const uint16_t buf_ln = BLOCKSIZE_LARGE;
	size_t n = buf_ln;
	size_t read_size = buf_ln;
	size_t block_i, j;
	size_t found = FIND_FAILURE;
//	printf("findNeedleInFP(0x%lx, 0x%lx)\n", offset, max_offset);

	j = 0;

	while ( n == buf_ln )
	{
//		printf("\r0x%lx", offset);
		if ( offset + read_size > max_offset )
			read_size = max_offset - offset;

		fseek(fi, offset, SEEK_SET);
		n = fread(buf, 1, read_size, fi);

		block_i = findNeedleInBlock(needle, needle_ln, buf, &j, n);

		if ( j == needle_ln )
		{
			found = offset + block_i - needle_ln + 1;
			break;
		}

		offset += block_i;
	}
	return found;
}

/**
 * Find the needle in a loaded block.
 * If the needle has been found, j==needle_ln and the found offset in the block is the returned value.
 *
 * @param needle unsigned char* the needle
 * @param needle_ln uint32_t length of the needle
 * @param buf unsigned char* haystack buffer to find the needle in
 * @param j
 * @param n
 * @return	size_t the last search offset in the block.
 */
size_t findNeedleInBlock(const unsigned char* needle, uint32_t needle_ln, const unsigned char* buf, size_t* j, size_t n)
{
	size_t i;

	for ( i = 0; i < n; i++ )
	{
		while ( (*j) > 0 && needle[(*j)] != buf[i] )
		{
			(*j) = failure[(*j) - 1];
		}
		if ( needle[(*j)] == buf[i] )
		{
			(*j)++;
		}
		if ( (*j) == needle_ln )
			break;
		if ( (*j) == 0 && i > n - needle_ln )
			break;
	}
	return i;
}

/**
 * Computes the failure function using a boot-strapping process,
 * where the pattern is matched against itself.
 *
 * @param pattern unsigned char*
 * @param pattern_ln size_t
 * @param failure uint16_t
 */
void computeFailure(const unsigned char* pattern, size_t pattern_ln, uint16_t* failure)
{
	uint32_t i = 0, j = 0;

	for ( i = 1; i < pattern_ln; i++ )
	{
		while ( j > 0 && pattern[j] != pattern[i] )
		{
			j = failure[j - 1];
		}
		if ( pattern[j] == pattern[i] )
		{
			j++;
		}
		failure[i] = j;
	}
}

void Finder_cleanUp()
{
	if ( failure != NULL ) free(failure);
}
