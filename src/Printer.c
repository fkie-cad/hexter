#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <memory.h>
#include <stdint.h>
#include <inttypes.h>
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
#define UNICODE_GAP " "

#define BLANK_GAP_C ' '
#define SEPARATOR_GAP_C '-'

typedef void (*PRINT_COL)(const uint8_t*, size_t, size_t, uint16_t);

static void printBlockLoop(size_t nr_of_parts, uint8_t* block, FILE* fi, uint16_t block_size, size_t block_start, size_t block_max);

//static void printDoubleCols(const uint8_t* block, size_t size, void (*printCol)(const uint8_t*, size_t, size_t, uint16_t));
static void printDoubleCols(const uint8_t* block, size_t size, PRINT_COL printCol);

static void printTripleCols(const uint8_t* block, size_t size, size_t offset, uint8_t width, void (*printCol)(const uint8_t*, size_t, size_t, uint16_t));

static void fillGap(uint8_t k);

static void printAsciiCols(const uint8_t* block, size_t size, uint16_t col_size);
static void printAsciiCol(const uint8_t* block, size_t i, size_t size, uint16_t col_size);

static void printUnicodeCols(const uint8_t* block, size_t size, uint16_t col_size);
static void printUnicodeCol(const uint8_t* block, size_t i, size_t size, uint16_t col_size);

void printPlainByteString(const uint8_t* block, size_t size);

static void printHexCols(const uint8_t* block, size_t size);
static uint8_t printHexCol(const uint8_t* block, size_t i, size_t size, uint8_t col_size);
static uint8_t printHexCol16(const uint8_t* block, size_t i, size_t size, uint8_t col_size);

static void printOffsetCol(size_t offset, uint8_t width);
static void printCleanHexValue(const uint64_t b, const uint8_t width, const char gap);
static void printAnsiFormatedHexValue(const uint64_t b, const uint8_t width, const char gap);
#ifdef _WIN32
static void printWinFormatedHexValue(const uint64_t b, const uint8_t width, const char gap);
#endif

static void printAsciiChar(
    const uint8_t c
);

static void printUnicodeChar(
    const uint16_t c
);

void (*printHexValue)(const uint64_t, const uint8_t, const char);

#if defined(_WIN32)
    HANDLE hStdout;
    WORD wOldColorAttrs;
#endif

static int8_t skip_hex_bytes = 0;
static int8_t skip_ascii_bytes = 0;
static int8_t skip_unicode_bytes = 0;

static uint32_t highlight_hex_bytes = 0;
static int32_t highlight_hex_wait = 0;
static uint32_t highlight_ascii_bytes = 0;
static uint32_t highlight_unicode_bytes = 0;
static int32_t highlight_ascii_wait = 0;
static int32_t highlight_unicode_wait = 0;

static uint8_t* needle = NULL;
static uint32_t needle_ln;
static size_t found = 0;
static uint32_t find_flags = 0;


/**
 * Prints the values depending on the mode.
 *
 * If block_size % col_size != 0 some more adjustments have to be taken to the col printings.
 * I.e. the index has to be passed and returned and the new line has to check for block and size.
 */
void print(size_t start, uint8_t skip_bytes, uint8_t* _needle, uint32_t _needle_ln)
{
    needle = _needle;
    needle_ln = _needle_ln;
    
    int errsv;
    FILE* fi;
    uint8_t* block = NULL;
    size_t block_start = start;
    uint16_t block_size = BLOCKSIZE_LARGE;
    size_t nr_of_parts = length / block_size;
    if ( length % block_size != 0 ) nr_of_parts++;

    if ( (mode_flags&(MODE_FLAG_FIND|MODE_FLAG_CASE_INSENSITIVE)) == (MODE_FLAG_FIND|MODE_FLAG_CASE_INSENSITIVE) )
        find_flags = (FIND_FLAG_CASE_INSENSITIVE|FIND_FLAG_ASCII);

    debug_info("start: 0x%zx\n", start);
    debug_info("block_size: 0x%x\n", block_size);
    debug_info("nr_of_parts: 0x%zx\n", nr_of_parts);
    debug_info("mode_flags: 0x%x\n", mode_flags);
    debug_info("find_flags: 0x%x\n", find_flags);
    debug_info("\n");

    errno = 0;
    fi = fopen(file_path, "rb");
    errsv = errno;
    if ( !fi )
    {
        printf("ERROR (0x%x): Could not open \"%s\".\n", errsv, file_path);
        return;
    }

    block = (uint8_t*) malloc(block_size);
    if ( !block )
    {
        printf("Malloc block failed.\n");
        fclose(fi);
        return;
    }

    Printer_setSkipBytes(skip_bytes);

    if ( mode_flags&MODE_FLAG_FIND )
    {
        Finder_initFailure(needle, needle_ln);
        found = findNeedleInFile(file_path, needle, needle_ln, start, file_size, find_flags);
        //found = findNeedleInFP(needle, needle_ln, found+needle_ln, fi, file_size);
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

    if ( (mode_flags&MODE_FLAG_CONTINUOUS_PRINTING) && (block_start < file_size) )
        printBlockLoop(nr_of_parts, block, fi, block_size, block_start, file_size);

    Printer_cleanUp(block, fi);
}

void Printer_setSkipBytes(uint8_t skip_bytes)
{
    if ( skip_bytes > 0 )
        skip_hex_bytes = skip_ascii_bytes = skip_unicode_bytes = skip_bytes;
}

void setPrintingStyle()
{
#ifdef CLEAN_PRINTING
    printHexValue = &printCleanHexValue;
#elif defined(__linux__) || defined(__linux) || defined(linux)
    if ( (mode_flags&MODE_FLAG_CLEAN_PRINTING) || !isatty(fileno(stdout)) )
        printHexValue = &printCleanHexValue;
    else
        printHexValue = &printAnsiFormatedHexValue;
#elif defined(_WIN32)
    if ( (mode_flags&MODE_FLAG_CLEAN_PRINTING) || !_isatty(_fileno(stdout)) )
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

void Printer_cleanUp(uint8_t* block, FILE* fi)
{
    free(block);
    fclose(fi);
    Finder_cleanUp();
}

void printBlockLoop(size_t nr_of_parts, uint8_t* block, FILE* fi, uint16_t block_size, size_t block_start, size_t block_max)
{
    char input;
    uint8_t skip_bytes = 0;

    while ( 1 )
    {
        input = (char)_getch();

        if ( input == ENTER )
            block_start = printBlock(nr_of_parts, block, fi, block_size, block_start, block_max);
        else if ( (mode_flags&MODE_FLAG_FIND) && input == NEXT )
        {
            found = findNeedleInFP(needle, needle_ln, found+needle_ln, fi, block_max, find_flags);
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

size_t printBlock(size_t nr_of_parts, uint8_t* block, FILE* fi, uint16_t block_size, size_t read_start, size_t read_max)
{
    size_t p;
    size_t read_size = 0;
    size_t size;
    size_t end = read_start + length;
    uint8_t offset_width = countHexWidth64((end>HEX_COL_SIZE)?end-HEX_COL_SIZE:end);
    int errsv = 0;

    // adjust end size if it exceeds read_max
    if ( end > read_max )
        end = read_max;

    for ( p = 0; p < nr_of_parts; p++ )
    {
        read_size = block_size;
#ifdef DEBUG_PRINT
        debug_info("%zu / %zu\n", (p+1), nr_of_parts);
        debug_info(" - read_size: 0x%zx\n", read_size);
        debug_info(" - block_start: 0x%zx\n", read_start);
        debug_info(" - end: 0x%zx\n", end);
#endif
        if ( read_start >= end )
            break;
        if ( read_start + read_size > end )
            read_size = end - read_start;
        if ( read_size == 0 )
            break;
#ifdef DEBUG_PRINT
        debug_info(" - read_size: 0x%zx\n", read_size);
#endif

        memset(block, 0, block_size);
        size = readFile(fi, read_start, read_size, block, &errsv);

        if ( !size )
        {
            printf("ERROR (0x%x): Reading block of bytes failed!\n", errsv);
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

void printLine(const uint8_t* block, size_t block_start, size_t size, uint8_t offset_width)
{
    if ( print_col_mask == (PRINT_OFFSET_MASK | PRINT_ASCII_MASK | PRINT_HEX_MASK) )
        printTripleCols(block, size, block_start, offset_width, printAsciiCol);
    else if ( print_col_mask == (PRINT_OFFSET_MASK | PRINT_UNICODE_MASK | PRINT_HEX_MASK) )
        printTripleCols(block, size, block_start, offset_width, printUnicodeCol);
    else if ( print_col_mask == (PRINT_ASCII_MASK | PRINT_HEX_MASK) )
        printDoubleCols(block, size, printAsciiCol);
    else if ( print_col_mask == (PRINT_UNICODE_MASK | PRINT_HEX_MASK) )
        printDoubleCols(block, size, printUnicodeCol);
    else if ( print_col_mask == PRINT_ASCII_MASK )
        printAsciiCols(block, size, ASCII_COL_SIZE);
    else if ( print_col_mask == PRINT_HEX_MASK )
        printHexCols(block, size);
    else if ( print_col_mask == PRINT_UNICODE_MASK )
        printUnicodeCols(block, size, UNICODE_COL_SIZE);
    else if ( print_col_mask == PRINT_BYTES_STRING )
        printPlainByteString(block, size);
}

//void printDoubleCols(const uint8_t* block, size_t size, void (*printCol)(const uint8_t*, size_t, size_t, uint16_t))
void printDoubleCols(const uint8_t* block, size_t size, PRINT_COL printCol)
{
    size_t i;
    uint8_t k = 0;

    for ( i = 0; i < size; i += HEX_COL_SIZE )
    {
        k = printHexCol(block, i, size, HEX_COL_SIZE);

        fillGap(k);

        printf("%c ", COL_SEPARATOR);

        printCol(block, i, size, HEX_COL_SIZE);

        printf("\n");
    }
}

void printTripleCols(const uint8_t* block, size_t size, size_t offset, uint8_t width, void (*printCol)(const uint8_t*, size_t, size_t, uint16_t))
{
    size_t i;
    uint8_t k = 0;

    for ( i = 0; i < size; i += TRIPLE_COL_SIZE )
    {
        printOffsetCol(offset, width);

        k = printHexCol(block, i, size, TRIPLE_COL_SIZE);

        fillGap(k);

        printf("%c ", COL_SEPARATOR);
        
        printCol(block, i, size, TRIPLE_COL_SIZE);

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

void printAsciiCols(const uint8_t* block, size_t size, uint16_t col_size)
{
    size_t i;

    for ( i = 0; i < size; i += col_size )
    {
        printAsciiCol(block, i, size, col_size);
        printf("\n");
    }
}

void printAsciiCol(const uint8_t* block, size_t i, size_t size, uint16_t col_size)
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

        printAsciiChar(block[temp_i]);
    }
}



void printUnicodeCols(const uint8_t* block, size_t size, uint16_t col_size)
{
    size_t i;

    for ( i = 0; i < size; i += col_size )
    {
        printUnicodeCol(block, i, size, col_size);
        printf("\n");
    }
}

void printUnicodeCol(const uint8_t* block, size_t i, size_t size, uint16_t col_size)
{
    size_t k = 0;
    size_t temp_i;

    for ( k = 0; k < col_size; k+=2 )
    {
        temp_i = i + k;
        if ( temp_i+1 >= size )
            break;

        if ( skip_unicode_bytes > 0 )
        {
            printf(UNICODE_GAP);
            skip_unicode_bytes-=2;
            continue;
        }

        printUnicodeChar(*(uint16_t*)&block[temp_i]);
    }
}



void printHexCols(const uint8_t* block, size_t size)
{
    size_t i;

    for ( i = 0; i < size; i += HEX_COL_SIZE )
    {
        printHexCol(block, i, size, HEX_COL_SIZE);

        printf("\n");
    }
}

uint8_t printHexCol(const uint8_t* block, size_t i, size_t size, uint8_t col_size)
{
    uint8_t k = 0;
    size_t block_offset;
    char gap = BLANK_GAP_C;
    uint8_t gap_ctr = 0;

    for ( k = 0, gap_ctr=0; k < col_size; k++, gap_ctr++ )
    {
        block_offset = i + k;
        if ( block_offset >= size )
            break;

        if ( skip_hex_bytes > 0 )
        {
            printf(HEX_GAP);
            skip_hex_bytes--;
            continue;
        }

        if ( (gap_ctr+1) == col_size/2 )
            gap = SEPARATOR_GAP_C;
        else 
            gap = BLANK_GAP_C;
        if ( (gap_ctr+1) == col_size )
            gap_ctr = 0;

        printHexValue(block[block_offset], 2, gap);
    }

    return k;
}

uint8_t printHexCol16(const uint8_t* block, size_t i, size_t size, uint8_t col_size)
{
    uint8_t k = 0;
    size_t block_offset;
    char gap = BLANK_GAP_C;
    uint8_t gap_ctr = 0;

    for ( k = 0, gap_ctr=0; k < col_size; k+=2, gap_ctr+=2 )
    {
        block_offset = i + k;
        if ( block_offset >= size )
            break;

        if ( skip_hex_bytes > 0 )
        {
            printf(HEX_GAP);
            skip_hex_bytes--;
            continue;
        }

        if ( (gap_ctr+1) == col_size/2 )
            gap = SEPARATOR_GAP_C;
        else 
            gap = BLANK_GAP_C;
        if ( (gap_ctr+1) == col_size )
            gap_ctr = 0;

        printHexValue(*(uint16_t*)&block[block_offset], 4, gap);
    }

    return k;
}

void printPlainByteString(const uint8_t* block, size_t size)
{
    size_t k = 0;

    for ( k = 0; k < size; k++ )
    {
        if ( skip_hex_bytes > 0 )
        {
            printf(HEX_GAP);
            skip_hex_bytes--;
            continue;
        }
          
        printf("%02x", block[k]);
    }
}

void printCleanHexValue(const uint64_t v, const uint8_t width, const char gap)
{
    printf("%0*"PRIX64"%c", width, v, gap);
}

void printAnsiFormatedHexValue(const uint64_t v, const uint8_t width, const char gap)
{
    if ( highlight_hex_bytes > 0 && highlight_hex_wait-- <= 0 )
    {
        setAnsiFormat(HIGHLIGHT_HEX_STYLE);
        highlight_hex_bytes--;
        printCleanHexValue(v, width, gap);
        resetAnsiFormat();
    }
    else if ( v == 0 )
    {
        printCleanHexValue(v, width, gap);
    }
    else
    {
        setAnsiFormat(POS_HEX_STYLE);
        printCleanHexValue(v, width, gap);
        resetAnsiFormat();
    }
}

#ifdef _WIN32
void printWinFormatedHexValue(const uint64_t v, const uint8_t width, const char gap)
{    
    if ( highlight_hex_bytes > 0 && highlight_hex_wait-- <= 0 )
    {
        SetConsoleTextAttribute(hStdout, BACKGROUND_INTENSITY);
        highlight_hex_bytes--;
        printCleanHexValue(v,  width, gap);
        SetConsoleTextAttribute(hStdout, wOldColorAttrs);
    }
    else if ( v == 0 )
    {
        SetConsoleTextAttribute(hStdout, FOREGROUND_INTENSITY);
        printCleanHexValue(v,  width, gap);
        SetConsoleTextAttribute(hStdout, wOldColorAttrs);
    }
    else
    {
        printCleanHexValue(v,  width, gap);
    }
}
#endif

void printAsciiChar(const uint8_t c)
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

void printUnicodeChar(const uint16_t c)
{
    if ( highlight_unicode_bytes > 0  && highlight_unicode_wait <= 0 )
    {
#ifdef _WIN32
        SetConsoleTextAttribute(hStdout, BACKGROUND_INTENSITY);
#else
        printf(HIGHLIGHT_HEX_STYLE);
#endif
    }

    if ( c == 0 || c == '\n' || c == '\r' || c == '\t')
        printf("%lc", NO_PRINT_UC_SUBSTITUTION);
    else
        printf("%lc", c);

    if ( highlight_unicode_bytes > 0 && highlight_unicode_wait <= 0)
    {
        highlight_unicode_bytes -= 2;
#ifdef _WIN32
        SetConsoleTextAttribute(hStdout, wOldColorAttrs);
#else
        resetAnsiFormat();
#endif
    }
    highlight_unicode_wait -= 2;
}

void Printer_setHighlightBytes(uint32_t v)
{
    highlight_hex_bytes = v;
    highlight_ascii_bytes = v;
    highlight_unicode_bytes = v;
}

void Printer_setHighlightWait(uint32_t v)
{
    highlight_hex_wait = v;
    highlight_ascii_wait = v;
}
