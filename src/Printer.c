#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(__linux__) || defined(__linux) || defined(linux)
	#include <unistd.h>
	#include "utils/TerminalUtil.h"
#endif
#if defined(_WIN32)
	#include <conio.h>
	#include <io.h>
	#include <windows.h>
#endif

#include "Printer.h"
#include "Globals.h"
#include "Finder.h"
#include "utils/common_fileio.h"
#include "utils/Helper.h"

#define HEX_GAP "   "
#define ASCII_GAP " "

void printAsciiByte(const unsigned char c);
void (*printHexValue)(uint8_t);

#if defined(_WIN32)
	HANDLE hStdout;
	WORD wOldColorAttrs;
#endif

static int8_t skip_hex_bytes = 0;
static int8_t skip_ascii_bytes = 0;

static int16_t highlight_hex_bytes = 0;
static int16_t highlight_hex_wait = 0;
static int16_t highlight_ascii_bytes = 0;
static int16_t highlight_ascii_wait = 0;

static unsigned char* needle = NULL;
static uint32_t needle_ln;
static uint64_t found = 0;

/**
 * Prints the values depending on the mode.
 *
 * If block_size % col_size != 0 some more adjustments have to be taken to the col printings.
 * I.e. the index has to be passed and returned and the new line has to check for block and size.
 */
void print(uint64_t start, uint8_t skip_bytes, unsigned char* _needle, uint32_t _needle_ln)
{
	needle = _needle;
	needle_ln = _needle_ln;

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

	Printer_setSkipBytes(skip_bytes);

	if ( find_f )
	{
		Finder_initFailure(needle, needle_ln);
		found = findNeedleInFile(file_path, needle, needle_ln, start, file_size);
		if ( found == FIND_FAILURE )
		{
			Printer_cleanUp(block, fi);
			return;
		}

		block_start = normalizeOffset(found, &skip_bytes);
		Printer_setHiglightBytes(needle_ln);
		Printer_setHiglightWait(skip_bytes);
		skip_bytes = 0;
	}

	block_start = printBlock(nr_of_parts, block, fi, block_size, block_start, file_size);

	if ( continuous_f && block_start < file_size )
		printBlockLoop(nr_of_parts, block, fi, block_size, block_start, file_size);

	Printer_cleanUp(block, fi);
}

void Printer_setSkipBytes(uint8_t skip_bytes)
{
	if ( skip_bytes > 0 )
		skip_hex_bytes = skip_ascii_bytes = skip_bytes;
}

void setPrintingStyle()
{
#ifdef CLEAN_PRINTING
	printHexValue = &printCleanHexValue;
#elif defined(__linux__) || defined(__linux) || defined(linux)
	if ( clean_printing || !isatty(fileno(stdout)) )
		printHexValue = &printCleanHexValue;
	else
		printHexValue = &printAnsiFormatedHexValue;
#elif defined(_WIN32)
	if ( clean_printing || !_isatty(_fileno(stdout)) )
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
}

void Printer_cleanUp(unsigned char* block, FILE* fi)
{
	free(block);
	fclose(fi);
	Finder_cleanUp();
}

void printBlockLoop(uint64_t nr_of_parts, unsigned char* block, FILE* fi, uint16_t block_size, uint64_t block_start, uint64_t block_max)
{
	char input;
	uint8_t skip_bytes = 0;

	while ( 1 )
	{
		input = getch();

		if ( input == ENTER )
			block_start = printBlock(nr_of_parts, block, fi, block_size, block_start, block_max);
		else if ( find_f && input == 'n' )
		{
			found = findNeedleInFP(needle, needle_ln, found+needle_ln, fi, block_max);
			if ( found == FIND_FAILURE )
				break;

			block_start = normalizeOffset(found, &skip_bytes);
			Printer_setHiglightBytes(needle_ln);
			Printer_setHiglightWait(skip_bytes);
			skip_bytes = 0;

			printf("\n");
			block_start = printBlock(nr_of_parts, block, fi, block_size, block_start, block_max);
		}
		else if ( input == 'q' )
			break;

		if ( block_start == UINT64_MAX )
			break;
	}
}

uint64_t printBlock(uint64_t nr_of_parts, unsigned char* block, FILE* fi, uint16_t block_size, uint64_t block_start, uint64_t block_max)
{
	uint64_t p;
	uint64_t read_size = 0;
	uint64_t size;
	uint64_t end = block_start + length;
	uint8_t offset_width = countHexWidth64(end);

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
			printf("ERROR: Reading block of bytes failed!\n");
			block_start = block_max;
			break;
		}

		printLine(block, block_start, size, offset_width);

		block_start += read_size;
	}

	if ( block_start >= block_max )
		block_start = UINT64_MAX;

	return block_start;
}

void printLine(const unsigned char* block, uint64_t block_start, uint64_t size, uint8_t offset_width)
{
	if ( print_col_mask == (print_offset_mask | print_ascii_mask | print_hex_mask) )
		printTripleCols(block, size, block_start, offset_width);
	else if ( print_col_mask == (print_ascii_mask | print_hex_mask) )
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

void printTripleCols(const unsigned char* block, uint64_t size, uint64_t offset, uint8_t width)
{
	uint64_t i;
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
#if defined(_WIN32)
	printf("%0*llx: ", width, offset);
#else
	printf("%0*lx: ", width, offset);
#endif
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

		printAsciiByte(block[temp_i]);
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
	if ( highlight_hex_bytes > 0 && highlight_hex_wait-- <= 0 )
	{
		printf(HIGHLIGHT_HEX_STYLE);
		highlight_hex_bytes--;
		printf("%02X ", b);
		resetAnsiFormat();
	}
	else if ( b == 0 )
	{
		printf("%02X ", b);
	}
	else
	{
		setAnsiFormat(POS_HEX_STYLE);
		printf("%02X ", b);
		resetAnsiFormat();
	}
}

#ifdef _WIN32
void printWinFormatedHexValue(const unsigned char b)
{
	if ( highlight_hex_bytes > 0 && highlight_hex_wait-- <= 0 )
	{
		SetConsoleTextAttribute(hStdout, BACKGROUND_INTENSITY);
		highlight_hex_bytes--;
		printf("%02X ", b);
    	SetConsoleTextAttribute(hStdout, wOldColorAttrs);
	}
	else if ( b == 0 )
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

void printAsciiByte(const unsigned char c)
{
	if ( highlight_ascii_bytes > 0  && highlight_ascii_wait <= 0 )
	{
#ifdef _WIN32
		SetConsoleTextAttribute(hStdout, BACKGROUND_INTENSITY);
#else
		printf(HIGHLIGHT_HEX_STYLE);
#endif
	}

	if ( MIN_PRINTABLE_ASCII_RANGE <= c && c <= MAX_PRINTABLE_ASCII_RANGE )
		printf("%c", c);
	else
		printf("%c", NO_PRINT_ASCII_SUBSTITUTION);

	if ( highlight_ascii_bytes > 0 && highlight_ascii_wait-- <= 0)
	{
		highlight_ascii_bytes--;
#ifdef _WIN32
		SetConsoleTextAttribute(hStdout, wOldColorAttrs);
#else
		resetAnsiFormat();
#endif
	}
}

void Printer_setHiglightBytes(int16_t v)
{
	highlight_hex_bytes = v;
	highlight_hex_bytes = v;
}

void Printer_setHiglightWait(int16_t v)
{
	highlight_hex_wait = v;
	highlight_ascii_wait = v;
}
