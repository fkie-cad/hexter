#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(__linux__) || defined(__linux) || defined(linux)
	#include <unistd.h>
	#include "utils/TerminalUtil.h"
	#define ENTER 10 // aka \n
#endif
#if defined(_WIN32)
	#include <conio.h>
	#include <windows.h>
	#define ENTER 13 // aka \r
#endif

#include "Printer.h"
#include "Globals.h"
#include "utils/common_fileio.h"
#include "utils/Helper.h"

#define HEX_GAP "   "
#define ASCII_GAP " "

void (*printHexValue)(uint8_t);

uint64_t printSkipBytes(uint64_t start, FILE* fi, unsigned char* block, uint64_t block_start, uint16_t block_size);

#if defined(_WIN32)
	HANDLE hStdout;
	WORD wOldColorAttrs;
#endif

uint8_t skip_hex_bytes = 0;
uint8_t skip_ascii_bytes = 0;

/**
 * Prints the values depending on the mode.
 *
 * If block_size % col_size != 0 some more adjustments have to be taken to the col printings.
 * I.e. the index has to be passed and return and the new line has to check for block and size.
 */
void print(uint64_t start, uint8_t skip_bytes)
{
	FILE* fi;
	unsigned char* block = NULL;
	uint64_t block_start = start;
	uint16_t block_size = BLOCKSIZE_LARGE;
	uint64_t nr_of_parts = length / block_size;
	if ( length % block_size != 0 ) nr_of_parts++;

	debug_info("start: %lu\n", start);
	debug_info("block_size: %d\n", block_size);
	debug_info("nr_of_parts: %lu\n", nr_of_parts);
	debug_info("\n");

	fi = fopen(file_path, "rb");
	if ( !fi )
	{
		printf("File %s does not exist.\n", file_path);
		return;
	}

	block = (unsigned char*) malloc(block_size);
	if ( !block )
	{
		printf("Malloc block failed.\n");
		return;
	}

#ifdef CLEAN_PRINTING
	printHexValue = &printCleanHexValue;
#elif defined(__linux__) || defined(__linux) || defined(linux)
	if ( clean_printing || !isatty(fileno(stdout)) )
		printHexValue = &printCleanHexValue;
	else
		printHexValue = &printAnsiFormatedHexValue;
#elif defined(_WIN32)
	if ( clean_printing || !isatty(fileno(stdout)) )
		printHexValue = &printCleanHexValue;
	else
	{
		hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
		GetConsoleScreenBufferInfo(hStdout, &csbiInfo);
		wOldColorAttrs = csbiInfo.wAttributes;
		printHexValue = &printWinFormatedHexValue;
	}
#else
	printHexValue = &printCleanHexValue;
#endif

	if ( skip_bytes > 0 )
		skip_hex_bytes = skip_ascii_bytes = skip_bytes;

	block_start = printBlock(nr_of_parts, block, fi, block_size, block_start);

	if ( continuous_f && block_start < file_size )
		printBlockLoop(nr_of_parts, block, fi, block_size, block_start);

	free(block);
	fclose(fi);
}

void printBlockLoop(uint64_t nr_of_parts, unsigned char* block, FILE* fi, uint16_t block_size, uint64_t block_start)
{
	char input;

	while ( 1 )
	{
		input = getch();

		if ( input == ENTER )
			block_start = printBlock(nr_of_parts, block, fi, block_size, block_start);
		else
			break;

		if ( block_start == UINT64_MAX )
			break;
	}
}

uint64_t printBlock(uint64_t nr_of_parts, unsigned char* block, FILE* fi, uint16_t block_size, uint64_t block_start)
{
	uint64_t p;
	uint64_t read_size = 0;
	uint64_t size;
	uint64_t end = block_start + length;
	uint8_t offset_width = countHexWidth64(end);
	debug_info("offset_width %u\n", offset_width);
	debug_info("block_start: %lu\n", block_start);
	debug_info("end: %lu\n", end);

	for ( p = 0; p < nr_of_parts; p++ )
	{
		debug_info("%lu / %lu\n", (p+1), nr_of_parts);
		read_size = block_size;
		if ( block_start + read_size > end ) read_size = end - block_start;
		debug_info(" - read_size: %lu\n", read_size);

		memset(block, 0, block_size);
		size = readFile(fi, block_start, read_size, block);
		if ( !size )
		{
			fprintf(stderr, "Reading block of bytes failed!\n");
			break;
		}

		printLine(block, block_start, size, offset_width);

		block_start += read_size;
	}

	if ( block_start >= file_size )
		block_start = UINT64_MAX;

	return block_start;
}

void printLine(const unsigned char* block, uint64_t block_start, uint64_t size, uint8_t offset_width)
{
	if ( print_col_mask == (print_offset_mask | print_ascii_mask | print_hex_mask))
		printTripleCols(block, size, block_start, offset_width);
	else if ( print_col_mask == (print_ascii_mask | print_hex_mask))
		printDoubleCols(block, size);
	else if ( print_col_mask == print_ascii_mask )
		printAsciiCols(block, size);
	else if ( print_col_mask == print_hex_mask )
		printHexCols(block, size);
}

void printDoubleCols(const unsigned char* block, uint64_t size)
{
	uint64_t i;
	uint8_t k = 0;

	for ( i = 0; i < size; i += DOUBLE_COL_SIZE )
	{
		k = printHexCol(block, i, size, DOUBLE_COL_SIZE);

		fillGap(k);

		printf("%c ", COL_SEPARATOR);

		printAsciiCol(block, i, size, DOUBLE_COL_SIZE);

		printf("\n");
	}
}

void printTripleCols(const unsigned char* block, uint64_t size, uint64_t start, uint8_t width)
{
	uint64_t i;
	uint64_t offset = start;
	uint8_t k = 0;

	for ( i = 0; i < size; i += TRIPLE_COL_SIZE )
	{
		printOffsetCol(offset, width);

		k = printHexCol(block, i, size, TRIPLE_COL_SIZE);

		fillGap(k);

		printf("%c ", COL_SEPARATOR);

		printAsciiCol(block, i, size, TRIPLE_COL_SIZE);

		printf("\n");

		offset += TRIPLE_COL_SIZE;
	}
}

void printOffsetCol(uint64_t offset, uint8_t width)
{
	printf("%0*lx: ", width, offset);
}

void fillGap(uint8_t k)
{
	uint8_t gap = DOUBLE_COL_SIZE - k;
	if ( gap > 0 )
	{
		for ( k = 0; k < gap; k++ )
		{
			printf(HEX_GAP);
		}
	}
}

void printAsciiCols(const unsigned char* block, uint64_t size)
{
	uint64_t i;

	for ( i = 0; i < size; i += ASCII_COL_SIZE )
	{
		printAsciiCol(block, i, size, ASCII_COL_SIZE);
		printf("\n");
	}
}

void printAsciiCol(const unsigned char* block, uint64_t i, uint64_t size, uint8_t col_size)
{
	uint64_t k = 0;
	uint64_t temp_i;
	char c;

	for ( k = 0; k < col_size; k++ )
	{
		temp_i = i + k;
		if ( temp_i >= size )
			break;

		if ( skip_ascii_bytes > 0 )
		{
			printf(ASCII_GAP);
			skip_ascii_bytes--;
			continue;
		}

		c = block[temp_i];
//		printf("[%d] %d|", temp_i, +c);
		if ( MIN_PRINTABLE_ASCII_RANGE <= c && c <= MAX_PRINTABLE_ASCII_RANGE )
			printf("%c", c);
		else
			printf("%c", NO_PRINT_ASCII_SUBSTITUTION);
	}
}

void printHexCols(const unsigned char* block, uint64_t size)
{
	uint64_t i;

	for ( i = 0; i < size; i += HEX_COL_SIZE )
	{
		printHexCol(block, i, size, HEX_COL_SIZE);

		printf("\n");
	}
}

uint8_t printHexCol(const unsigned char* block, uint64_t i, uint64_t size, uint8_t col_size)
{
	uint8_t k = 0;
	uint64_t temp_i;

	for ( k = 0; k < col_size; k++ )
	{
		temp_i = i + k;
		if ( temp_i >= size )
			break;

		if ( skip_hex_bytes > 0 )
		{
			printf(HEX_GAP);
			skip_hex_bytes--;
			continue;
		}

		(*printHexValue)(block[temp_i]);
	}

	return k;
}

void printCleanHexValue(uint8_t b)
{
	printf("%02X ", b);
}

void printAnsiFormatedHexValue(const unsigned char b)
{
	if ( b == 0 )
	{
		printf("%02X ", b);
	}
	else
	{
		setAnsiFormat("\033[1m"); //Set bold
//		printf("\033[1;30m"); //Set color
		printf("%02X ", b);
		resetAnsiFormat();
	}
}

void setAnsiFormat(char* format)
{
	printf(format);
}

void resetAnsiFormat()
{
	printf("\033[0m");
}

#ifdef _WIN32
void printWinFormatedHexValue(const unsigned char b)
{
	if ( b == 0 )
	{
		SetConsoleTextAttribute(hStdout, FOREGROUND_INTENSITY);
		printf("%02X ", b);
    	SetConsoleTextAttribute(hStdout, wOldColorAttrs);
	}
	else
	{
		printf("%02X ", b);
	}
}
#endif
