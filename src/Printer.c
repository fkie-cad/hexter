#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "Printer.h"
#include "Globals.h"
#include "common_fileio.h"

/**
 * Prints the values depending on the mode.
 *
 * If block_size % col_size != 0 some more adjustments have to be taken to the col printings.
 * I.e. the index has to be passed and return and the new line has to check for block and size.
 */
void print()
{
	uint8_t ascii_hex_print = (ascii_only == 0 && hex_only == 0) || (ascii_only == 1 && hex_only == 1);
	uint64_t p;
	uint64_t end = start + length;
	uint16_t block_size = BLOCKSIZE_LARGE;
	uint64_t block_start = start;
	uint64_t block_end = 0;
	uint64_t parts = length / block_size;
	if ( length % block_size != 0 ) parts++;

	unsigned char* block = NULL;

	debug_info("end: %d\n", end);
	debug_info("block_size: %d\n", block_size);
	debug_info("block_start: %lu\n", block_start);
	debug_info("block_end: %lu\n", block_end);
	debug_info("parts: %lu\n", parts);
	debug_info("\n");

	block = (unsigned char*) malloc(block_size);
	if ( !block )
	{
		printf("Malloc block failed.\n");
		return;
	}

	for ( p = 0; p < parts; p++ )
	{
		block_end = block_start + block_size;
		if ( block_end > end ) block_end = end;

		uint64_t size = readCharArrayFileNA(file_name, block, block_size, block_start, block_end);
		if ( !size )
		{
			fprintf(stderr, "Reading block of bytes failed!\n");
			break;
		}

		if ( ascii_hex_print )
			printDoubleCols(block, size);
		else if ( ascii_only == 1 )
			printAsciiCol(block, size);
		else if ( hex_only == 1 )
			printHexCol(block, size);

		block_start += block_size;
	}

	free(block);
}

void printDoubleCols(unsigned char* block, uint64_t size)
{
	uint64_t i;
	uint64_t k = 0;
	uint64_t temp_i;

	for ( i = 0; i < size; i += DOUBLE_COL_SIZE )
	{
		for ( k = 0; k < DOUBLE_COL_SIZE; k++ )
		{
			temp_i = i + k;
			if ( temp_i >= size )
				break;

			printf("%02X", block[temp_i]);
		}

		uint8_t gap = DOUBLE_COL_SIZE - k;
		if ( gap > 0 )
		{
			for ( k = 0; k < gap; k++ )
			{
				printf("  ");
			}
		}

		printf("%c", COL_SEPARATOR);

		for ( k = 0; k < DOUBLE_COL_SIZE; k++ )
		{
			temp_i = i + k;
			if ( temp_i >= size )
				break;

			char c = block[temp_i];
//			if ( MIN_PRINT_ASCII_RANGE <= c && c <= MAX_PRINT_ASCII_RANGE )
			if ( MIN_PRINT_ASCII_RANGE <= c )
				printf("%c", c);
			else
				printf("%c", NO_PRINT_ASCII_SUBSTITUTION);
		}
		printf("\n");
	}
}

void printAsciiCol(unsigned char* block, uint64_t size)
{
	uint64_t i;
	uint64_t k = 0;
	uint64_t temp_i;

	for ( i = 0; i < size; i += ASCII_COL_SIZE )
	{
		for ( k = 0; k < ASCII_COL_SIZE; k++ )
		{
			temp_i = i + k;
			if ( temp_i >= size )
				break;

//			unsigned char c = block[temp_i];
			char c = block[temp_i];
//			if ( MIN_PRINT_ASCII_RANGE <= c && c <= MAX_PRINT_ASCII_RANGE )
			if ( MIN_PRINT_ASCII_RANGE <= c )
				printf("%c", c);
			else
				printf("%c", NO_PRINT_ASCII_SUBSTITUTION);
		}
		printf("\n");
	}
}

void printHexCol(unsigned char* block, uint64_t size)
{
	uint64_t i;
	uint64_t k = 0;
	uint64_t temp_i;

	for ( i = 0; i < size; i += HEX_COL_SIZE )
	{
		for ( k = 0; k < HEX_COL_SIZE; k++ )
		{
			temp_i = i + k;
			if ( temp_i >= size )
				break;

			printf("%02X", block[temp_i]);
		}

		printf("\n");
	}
}
