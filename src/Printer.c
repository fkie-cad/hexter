#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
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

static uint32_t highlight_hex_bytes = 0;
static uint32_t highlight_hex_wait = 0;
static uint32_t highlight_ascii_bytes = 0;
static uint32_t highlight_ascii_wait = 0;

static unsigned char* needle = NULL;
static uint32_t needle_ln;
static size_t found = 0;

static int errsv;

/**
 * Prints the values depending on the mode.
 *
 * If block_size % col_size != 0 some more adjustments have to be taken to the col printings.
 * I.e. the index has to be passed and returned and the new line has to check for block and size.
 */
void print(size_t start, uint8_t skip_bytes, unsigned char* _needle, uint32_t _needle_ln)
{
	needle = _needle;
	needle_ln = _needle_ln;

	FILE* fi;
	unsigned char* block = NULL;
	size_t block_start = start;
	uint16_t block_size = BLOCKSIZE_LARGE;
	size_t nr_of_parts = length / block_size;
	if ( length % block_size != 0 ) nr_of_parts++;

	debug_info("start: 0x%zx\n", start);
	debug_info("block_size: 0x%x\n", block_size);
	debug_info("nr_of_parts: 0x%zx\n", nr_of_parts);
	debug_info("\n");

	errno = 0;
	fi = fopen(file_path, "rb");
	errsv = errno;
	if ( !fi )
	{
		printf("ERROR (0x%x): Could not open \"%s\".\n", errsv, file_path);
		return;
	}

	block = (unsigned char*) malloc(block_size);
	if ( !block )
	{
		printf("Malloc block failed.\n");
		fclose(fi);
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
		Printer_setHighlightBytes(needle_ln);
		Printer_setHighlightWait(skip_bytes);
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

void printBlockLoop(size_t nr_of_parts, unsigned char* block, FILE* fi, uint16_t block_size, size_t block_start, size_t block_max)
{
	char input;
	uint8_t skip_bytes = 0;

	while ( 1 )
	{
		input = (char)_getch();

		if ( input == ENTER )
			block_start = printBlock(nr_of_parts, block, fi, block_size, block_start, block_max);
		else if ( find_f && input == NEXT )
		{
			found = findNeedleInFP(needle, needle_ln, found+needle_ln, fi, block_max);
			if ( found == FIND_FAILURE )
				break;

			block_start = normalizeOffset(found, &skip_bytes);
			Printer_setHighlightBytes(needle_ln);
			Printer_setHighlightWait(skip_bytes);
			skip_bytes = 0;

			printf("\n");
			block_start = printBlock(nr_of_parts, block, fi, block_size, block_start, block_max);
		}
		else if ( input == QUIT )
			break;

		if ( block_start == SIZE_MAX )
			break;
	}
}

size_t printBlock(size_t nr_of_parts, unsigned char* block, FILE* fi, uint16_t block_size, size_t read_start, size_t read_max)
{
	size_t p;
	size_t read_size = 0;
	size_t size;
	size_t end = read_start + length;
	uint8_t offset_width = countHexWidth64(end-HEX_COL_SIZE);

	// adjust end size if it exceeds read_max
	if ( end > read_max )
		end = read_max;

	for ( p = 0; p < nr_of_parts; p++ )
	{
		debug_info("%zu / %zu\n", (p+1), nr_of_parts);
		read_size = block_size;
		debug_info(" - read_size: 0x%zx\n", read_size);
		debug_info(" - block_start: 0x%zx\n", read_start);
		debug_info(" - end: 0x%zx\n", end);
		if ( read_start >= end )
			break;
		if ( read_start + read_size > end )
			read_size = end - read_start;
		if ( read_size == 0 )
			break;
		debug_info(" - read_size: 0x%zx\n", read_size);

		memset(block, 0, block_size);
		size = readFile(fi, read_start, read_size, block);

		if ( !size )
		{
			printf("ERROR (0x%x): Reading block of bytes failed!\n", cfio_getErrno());
			read_start = read_max;
			break;
		}

		printLine(block, read_start, size, offset_width);

		read_start += read_size;
	}

	if ( read_start >= read_max )
		read_start = SIZE_MAX;

	return read_start;
}

void printLine(const unsigned char* block, size_t block_start, size_t size, uint8_t offset_width)
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

void printDoubleCols(const unsigned char* block, size_t size)
{
	size_t i;
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

void printTripleCols(const unsigned char* block, size_t size, size_t offset, uint8_t width)
{
	size_t i;
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

void printOffsetCol(size_t offset, uint8_t width)
{
	printf("%0*zx: ", width, offset);
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

void printAsciiCols(const unsigned char* block, size_t size)
{
	size_t i;

	for ( i = 0; i < size; i += ASCII_COL_SIZE )
	{
		printAsciiCol(block, i, size, ASCII_COL_SIZE);
		printf("\n");
	}
}

void printAsciiCol(const unsigned char* block, size_t i, size_t size, uint8_t col_size)
{
	size_t k = 0;
	size_t temp_i;

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

void printHexCols(const unsigned char* block, size_t size)
{
	size_t i;

	for ( i = 0; i < size; i += HEX_COL_SIZE )
	{
		printHexCol(block, i, size, HEX_COL_SIZE);

		printf("\n");
	}
}

uint8_t printHexCol(const unsigned char* block, size_t i, size_t size, uint8_t col_size)
{
	uint8_t k = 0;
	size_t temp_i;

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
		setAnsiFormat(HIGHLIGHT_HEX_STYLE);
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

void Printer_setHighlightBytes(uint32_t v)
{
	highlight_hex_bytes = v;
	highlight_ascii_bytes = v;
}

void Printer_setHighlightWait(uint32_t v)
{
	highlight_hex_wait = v;
	highlight_ascii_wait = v;
}
