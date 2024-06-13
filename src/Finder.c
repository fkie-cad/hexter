#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils/common_fileio.h"
#include "utils/Strings.h"
#include "Finder.h"
#include "Globals.h"

static uint16_t* failure = NULL;

void Finder_initFailure(uint8_t* needle, uint32_t needle_ln)
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
 * @param	needle uint8_t*
 * @param	needle_ln uint32_t
 * @param	offset size_t
 * @return	size_t the offset of the found needle or FIND_FAILURE, if not found
 */
size_t find(const char* path, const uint8_t* needle, uint32_t needle_ln, size_t offset, size_t max_offset, uint32_t flags)
{
    size_t found;

    uint16_t* f = NULL;
    f = (uint16_t*) calloc(needle_ln, sizeof(uint16_t));
    computeFailure(needle, needle_ln, f);

    found = findNeedleInFile(path, needle, needle_ln, offset, max_offset, flags);
//	uint8_t remainder = 0;

//	n_found = normalizeOffset(found, &remainder);
//	if ( found == FIND_FAILURE )
//		printf("Pattern not found!\n");
//	else
//		print(n_found, remainder);

    free(f);

    return found;
}

/**
 * KMPMatch
 *
 * @param needle uint8_t*
 * @param needle_ln uint32_t
 * @param offset size_t
 * @return
 */
size_t findNeedleInFile(const char* path, const uint8_t* needle, uint32_t needle_ln, size_t offset, size_t max_offset, uint32_t flags)
{
//	printf("BLOCKSIZE_LARGE: %u\n",BLOCKSIZE_LARGE);
    FILE* fi;
    size_t found;

    errno = 0;
    fi = fopen(path, "rb");
    int errsv = errno;
    if ( !fi )
    {
        printf("ERROR (0x%x): Could not open \"%s\".\n", errsv, path);
        return FIND_FAILURE;
    }

    found = findNeedleInFP(needle, needle_ln, offset, fi, max_offset, flags);

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
 * @return size_t the found offset or FIND_FAILURE
 */
size_t findNeedleInFP(const uint8_t* needle, uint32_t needle_ln, size_t offset, FILE* fi, size_t max_offset, uint32_t flags)
{
    uint8_t buf[BLOCKSIZE_LARGE];
    const uint16_t buf_ln = BLOCKSIZE_LARGE;
    size_t n = buf_ln;
    size_t read_size = buf_ln;
    size_t block_i, j;
    size_t found = FIND_FAILURE;

    //debug_info("findNeedleInFP(0x%zx, 0x%zx)\n", offset, max_offset);
    //debug_info("  flags: 0x%x\n", flags);

    j = 0;

    while ( n == buf_ln )
    {
//		printf("\r0x%lx", offset);
        if ( offset + read_size > max_offset )
            read_size = max_offset - offset;

        fseek(fi, offset, SEEK_SET);
        n = fread(buf, 1, read_size, fi);
        
        if ( (flags&(FIND_FLAG_CASE_INSENSITIVE|FIND_FLAG_ASCII)) == (FIND_FLAG_CASE_INSENSITIVE|FIND_FLAG_ASCII) )
        {
            toUpperCaseA((char*)buf, n);
        };

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
 * @param needle uint8_t* the needle
 * @param needle_ln uint32_t length of the needle
 * @param buf uint8_t* haystack buffer to find the needle in
 * @param j
 * @param n
 * @return	size_t the last search offset in the block.
 */
size_t findNeedleInBlock(const uint8_t* needle, uint32_t needle_ln, const uint8_t* buf, size_t* j, size_t n)
{
    size_t buf_i;
    size_t needle_i = *j;

    for ( buf_i = 0; buf_i < n; buf_i++ )
    {
        while ( needle_i > 0 && needle[needle_i] != buf[buf_i] )
        {
            needle_i = failure[needle_i - 1];
        }
        if ( needle[needle_i] == buf[buf_i] )
        {
            needle_i++;
        }
        if ( needle_i == needle_ln )
            break;
        if ( needle_i == 0 && buf_i > n - needle_ln )
            break;
    }
    
    *j = needle_i;

    return buf_i;
}

/**
 * Computes the failure function using a boot-strapping process,
 * where the pattern is matched against itself.
 *
 * @param pattern uint8_t*
 * @param pattern_ln size_t
 * @param failure uint16_t
 */
void computeFailure(const uint8_t* pattern, size_t pattern_ln, uint16_t* failure_)
{
    uint32_t i = 0, j = 0;

    for ( i = 1; i < pattern_ln; i++ )
    {
        while ( j > 0 && pattern[j] != pattern[i] )
        {
            j = failure_[j - 1];
        }
        if ( pattern[j] == pattern[i] )
        {
            j++;
        }
        failure_[i] = (uint16_t)j;
    }
}

void Finder_cleanUp()
{
    if ( failure != NULL ) free(failure);
}
