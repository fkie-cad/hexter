#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Finder.h"
#include "Globals.h"
#include "utils/common_fileio.h"

/**
 * KMPMatch
 *
 * @param needle
 * @param needle_ln
 * @return
 */
uint64_t find(const unsigned char* needle, uint32_t needle_ln)
{
//	printf("BLOCKSIZE_LARGE: %u\n",BLOCKSIZE_LARGE);
	unsigned char buf[BLOCKSIZE_LARGE];
	const int buf_ln = BLOCKSIZE_LARGE;
	int n = buf_ln;
	FILE* fi;
	uint64_t i, j, offset;
	uint16_t* failure;
	uint64_t found = -1;

	fi = fopen(file_name, "rb");
	if ( !fi )
	{
		printf("ERROR: File %s does not exist.\n", file_name);
		return -1;
	}

	offset = start;

	failure = (uint16_t*) malloc(needle_ln*sizeof(uint16_t));
	computeFailure(needle, needle_ln, failure);

	j = 0;

	while ( n == buf_ln )
	{
		fseek(fi, offset, SEEK_SET);
		n = fread(buf, 1, buf_ln, fi);

		for ( i = 0; i < buf_ln; i++)
		{
			while ( j > 0 && needle[j] != buf[i] )
			{
				j = failure[j - 1];
			}
			if ( needle[j] == buf[i] )
			{
				j++;
			}
			if ( j == needle_ln )
			{
				found = offset + i - needle_ln + 1;
				n = 0;
				break;
			}

			if ( j == 0 && i > n - needle_ln )
				break;
		}

		offset += i;
	}

	free(failure);
	fclose(fi);

	return found;
}

//int indexOf(byte[] data, byte[] pattern)
//{
//	if (data.length == 0) return -1;
//
//	int[] failure = new int[pattern.length];
//	computeFailure(pattern, failure);
//	int j = 0;
//
//	for (int i = 0; i < data.length; i++) {
//		while (j > 0 && pattern[j] != data[i]) {
//			j = failure[j - 1];
//		}
//		if (pattern[j] == data[i]) { j++; }
//		if (j == pattern.length) {
//			return i - pattern.length + 1;
//		}
//	}
//	return -1;
//}

/**
 * Computes the failure function using a boot-strapping process,
 * where the pattern is matched against itself.
 */
void computeFailure(const unsigned char* pattern, uint64_t pattern_ln, uint16_t* failure)
{
	uint16_t i = 0, j = 0;

	for ( i = 1; i < pattern_ln; i++)
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