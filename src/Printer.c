#include <memory.h>
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
	uint64_t read_size = 0;
	uint64_t parts = length / block_size;
	if ( length % block_size != 0 ) parts++;

	unsigned char* block = NULL;

	debug_info("start: %lu\n", start);
	debug_info("end: %lu\n", end);
	debug_info("block_size: %d\n", block_size);
	debug_info("block_start: %lu\n", block_start);
	debug_info("parts: %lu\n", parts);
	debug_info("\n");

	FILE* fi;
	fi = fopen(file_name, "rb");
	if ( !fi )
	{
		printf("File %s does not exist.\n", file_name);
		return;
	}

	block = (unsigned char*) malloc(block_size);
	if ( !block )
	{
		printf("Malloc block failed.\n");
		return;
	}

	for ( p = 0; p < parts; p++ )
	{
		debug_info("%lu / %lu\n", (p+1), parts);
		read_size = block_size;
		if ( block_start + read_size > end ) read_size = end - block_start;
		debug_info(" - read_size: %lu\n", read_size);

		memset(block, 0, block_size);
		uint64_t size = readFile(fi, block_start, read_size, block);
		if ( !size )
		{
			fprintf(stderr, "Reading block of bytes failed!\n");
			break;
		}

		if ( ascii_hex_print )
			printDoubleCols(block, size);
		else if ( ascii_only == 1 )
			printAsciiCols(block, size);
		else if ( hex_only == 1 )
			printHexCols(block, size);

		block_start += block_size;
	}

	free(block);
	fclose(fi);
}

void printDoubleCols(unsigned char* block, uint64_t size)
{
	uint64_t i;
	uint64_t k = 0;

	for ( i = 0; i < size; i += DOUBLE_COL_SIZE )
	{
		k = printHexCol(block, i, size, DOUBLE_COL_SIZE);

		fillGap(k);

		printf("%c ", COL_SEPARATOR);

		printAsciiCol(block, i, size, DOUBLE_COL_SIZE);

		printf("\n");
	}
}

void fillGap(uint64_t k)
{
	uint8_t gap = DOUBLE_COL_SIZE - k;
	if ( gap > 0 )
	{
		for ( k = 0; k < gap; k++ )
		{
			printf("   ");
		}
	}
}

void printAsciiCols(unsigned char* block, uint64_t size)
{
	uint64_t i;

	for ( i = 0; i < size; i += ASCII_COL_SIZE )
	{
		printAsciiCol(block, i, size, ASCII_COL_SIZE);
		printf("\n");
	}
}

void printAsciiCol(unsigned char* block, uint64_t i, uint64_t size, uint8_t col_size)
{
	uint64_t k = 0;
	uint64_t temp_i;

	for ( k = 0; k < col_size; k++ )
	{
		temp_i = i + k;
		if ( temp_i >= size )
			break;

		char c = block[temp_i];
		if ( MIN_PRINT_ASCII_RANGE <= c )
			printf("%c", c);
		else
			printf("%c", NO_PRINT_ASCII_SUBSTITUTION);
	}
}

void printHexCols(unsigned char* block, uint64_t size)
{
	uint64_t i;

	for ( i = 0; i < size; i += HEX_COL_SIZE )
	{
		printHexCol(block, i, size, HEX_COL_SIZE);

		printf("\n");
	}
}

uint64_t printHexCol(unsigned char* block, uint64_t i, uint64_t size, uint8_t col_size)
{
	uint64_t k = 0;
	uint64_t temp_i;

	for ( k = 0; k < col_size; k++ )
	{
		temp_i = i + k;
		if ( temp_i >= size )
			break;
#ifdef linux
		uint8_t b = block[temp_i];
		if ( b == 0)
		{
//			printf("\033[0;30m"); //Set the color
			printf("%02X ", b);
//			printf("\033[0m"); // reset
		}
		else
		{
			printf("\033[1m"); //Set bold
//			printf("\033[1;30m"); //Set color
			printf("%02X ", b);
			printf("\033[0m"); // reset
		}
#else
		printf("%02X ", block[temp_i]);
#endif
	}

	return k;
}
