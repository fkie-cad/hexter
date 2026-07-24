#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "utils/env.h"

#include <memory.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#if defined(_LINUX)
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
#define BYTE_STR_GAP "  "
#define ASCII_GAP " "
#define UNICODE_GAP " "

#define BLANK_GAP_C ' '
#define SEPARATOR_GAP_C '-'

//typedef void (*CharColPrinter)(const uint8_t*, size_t, size_t, uint32_t);

static void printBlockLoop(size_t nr_of_parts, uint8_t* buffer, FILE* fi, size_t buffer_size, size_t block_start, size_t block_max);

//static void printDoubleCols(const uint8_t* buffer, size_t size, void (*printCol)(const uint8_t*, size_t, size_t, uint16_t));
//static void printDoubleCols(const uint8_t* buffer, size_t size, CharColPrinter printCol);

static void printTripleCols(const uint8_t* buffer, size_t size, size_t offset, uint8_t width);

static void fillHexGap(uint32_t k, uint32_t col_size, uint32_t hex_size);

//static void printAsciiCols(const uint8_t* buffer, size_t size, uint32_t col_size);
static void printAsciiCol(const uint8_t* buffer, size_t i, size_t size, uint32_t col_size);

//static void printUnicodeCols(const uint8_t* buffer, size_t size, uint32_t col_size);
static void printUnicodeCol(const uint8_t* buffer, size_t i, size_t size, uint32_t col_size);

void printPlainByteString(const uint8_t* buffer, size_t size);

static void printHexCols(const uint8_t* buffer, size_t size);
static uint32_t printHexCol8(const uint8_t* buffer, size_t i, size_t size, uint32_t col_size);
static uint32_t printHexCol16(const uint8_t* buffer, size_t i, size_t size, uint32_t col_size);
static uint32_t printHexCol32(const uint8_t* buffer, size_t i, size_t size, uint32_t col_size);
static uint32_t printHexCol64(const uint8_t* buffer, size_t i, size_t size, uint32_t col_size);

static void printOffsetCol(size_t offset, uint32_t width);
static void printCleanHexValue(const uint64_t b, const uint32_t width, const char gap);
static void printAnsiFormatedHexValue(const uint64_t b, const uint32_t width, const char gap);
#ifdef _WIN32
static void printWinFormatedHexValue(const uint64_t b, const uint32_t width, const char gap);
#endif

static void printAsciiChar(
    const uint8_t c
);

static void printUnicodeChar(
    const uint16_t c
);

void (*printHexValue)(const uint64_t, const uint32_t, const char);

#if defined(_WIN32)
    HANDLE hStdout;
    WORD wOldColorAttrs;
#endif

static int32_t skip_hex_bytes = 0;
static int32_t skip_ascii_bytes = 0;
static int32_t skip_unicode_bytes = 0;

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
 * If buffer_size % col_size != 0 some more adjustments have to be taken to the col printings.
 * I.e. the index has to be passed and returned and the new line has to check for block and size.
 */
void print(size_t start, uint32_t skip_bytes, uint8_t* _needle, uint32_t _needle_ln)
{
    FEnter();

    needle = _needle;
    needle_ln = _needle_ln;
    
    int errsv;
    FILE* fi;
    uint8_t* buffer = NULL;
    size_t block_start = start;
    size_t buffer_size = BLOCK_SIZE;
    size_t nr_of_parts = g_length / buffer_size;
    if ( g_length % buffer_size != 0 ) nr_of_parts++;

    if ( ARE_FLAGS_SET(mode_flags, (MODE_FLAG_FIND|MODE_FLAG_CASE_INSENSITIVE)) )
        find_flags = (FIND_FLAG_CASE_INSENSITIVE|FIND_FLAG_ASCII);

    DPrint("start: 0x%zx\n", start);
    DPrint("skip_bytes: 0x%x\n", skip_bytes);
    DPrint("needle: %p\n", needle);
    DPrint("needle_ln: 0x%x\n", needle_ln);
    DPrint("buffer_size: 0x%zx\n", buffer_size);
    DPrint("nr_of_parts: 0x%zx\n", nr_of_parts);
    DPrint("mode_flags: 0x%x\n", mode_flags);
    DPrint("find_flags: 0x%x\n", find_flags);
    DPrint("\n");

    errno = 0;
    fi = fopen(file_path, "rb");
    errsv = errno;
    if ( !fi )
    {
        EPrint("Could not open \"%s\"! (0x%x)\n", file_path, errsv);
        return;
    }

    buffer = (uint8_t*)malloc(buffer_size);
    if ( !buffer )
    {
        errsv = errno;
        EPrint("Malloc buffer failed! (0x%x)\n", errsv);
        goto clean;
    }

    Printer_setSkipBytes(skip_bytes);
    if ( mode_flags&MODE_FLAG_FIND )
    {
        Finder_initFailure(needle, needle_ln, NULL);
    }
    
    printBlockLoop(nr_of_parts, buffer, fi, buffer_size, block_start, file_size);


clean:
    Printer_cleanUp(buffer, fi);

    FLeave();
}

void Printer_setSkipBytes(uint32_t skip_bytes)
{
    if ( skip_bytes > 0 )
        skip_hex_bytes = skip_ascii_bytes = skip_unicode_bytes = skip_bytes;
}

void setPrintingStyle()
{
    FEnter();
#ifdef CLEAN_PRINTING
    printHexValue = &printCleanHexValue;
#elif defined(_LINUX)
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
    FLeave();
}

void Printer_cleanUp(uint8_t* buffer, FILE* fi)
{
    FEnter();

    if ( buffer )
        free(buffer);
    if ( fi )
        fclose(fi);
    Finder_cleanUp();
    
    FLeave();
}

void printBlockLoop(size_t nr_of_parts, uint8_t* buffer, FILE* fi, size_t buffer_size, size_t block_start, size_t block_max)
{
    FEnter();

    uint32_t skip_bytes = 0;
    size_t find_offset = block_start;
    int continuing = (mode_flags&MODE_FLAG_CONTINUOUS_PRINTING) ? 1 : 0;
    char input = (mode_flags&MODE_FLAG_FIND) ? NEXT : ENTER;
    
    DPrint("nr_of_parts: 0x%zx\n", nr_of_parts);
    DPrint("buffer: %p\n", buffer);
    DPrint("fi: %p\n", fi);
    DPrint("buffer_size: 0x%zx\n", buffer_size);
    DPrint("block_start: 0x%zx\n", block_start);
    DPrint("block_max: 0x%zx\n", block_max);
    DPrint("find_offset: 0x%zx\n", find_offset);
    DPrint("file_size: 0x%zx\n", file_size);
    DPrint("input: %c\n", input);
    DPrint("continuing: %u\n", continuing);
    
    if ( block_start >= file_size )
    {
        IPrint("block_start exceeds file size!\n");
        return;
    }
    
    if ( (mode_flags&MODE_FLAG_PRINT_START_OFFSET) && !(mode_flags&MODE_FLAG_FIND) )
        printf("start: 0x%zx\n", block_start + skip_hex_bytes);

    do
    {
        if ( (mode_flags&MODE_FLAG_FIND) && (input == NEXT) )
        {
            found = findNeedleInFP(needle, needle_ln, find_offset, fi, block_max, find_flags);
            if ( found == FIND_FAILURE )
                break;
            find_offset = found+needle_ln;
            
            block_start = normalizeOffset(found, &skip_bytes, g_col_mask);
            Printer_setHighlightBytes(needle_ln);
            Printer_setHighlightWait(skip_bytes);
            skip_bytes = 0;

            printf("\n");
            DPrint("buffer_size: 0x%zx\n", buffer_size);
            DPrint("found: 0x%zx\n", found);
            DPrint("nr_of_parts: 0x%zx\n", nr_of_parts);

            // check if found needle exceeds current printing length
            size_t _length = g_length;
            size_t end = block_start + g_length;
            if ( found + needle_ln > end )
            {
                _length += needle_ln;
                _length = ALIGN_UP_BY(_length, g_length);
            }

            if ( mode_flags&MODE_FLAG_PRINT_START_OFFSET )
                printf("found: 0x%zx\n", found);
            block_start = printBlock(nr_of_parts, buffer, fi, buffer_size, block_start, block_max, _length);
        }
        else if ( input == ENTER )
        {
            block_start = printBlock(nr_of_parts, buffer, fi, buffer_size, block_start, block_max, g_length);
        }
        else if ( input == QUIT )
            break;
        
        // on break mode (-b) break;
        if ( !continuing )
            break;

        if ( block_start == SIZE_MAX )
            break;

        // find all always wants next
        if ( ARE_FLAGS_SET(mode_flags, (MODE_FLAG_FIND|MODE_FLAG_FIND_ALL)) )
            input = NEXT;
        // else wait for user decision
        else
            input = (char)_getch();
    }
    while ( continuing );

    FLeave();
}

//
// read in a min(buffer_size, g_length) of file bytes and print length bytes of it in a loop
// 
//
size_t printBlock(
    size_t nr_of_parts, 
    uint8_t* buffer, 
    FILE* fi, 
    size_t buffer_size, 
    size_t read_start, 
    size_t read_max,
    size_t length
)
{
    FEnter();

    size_t p;
    size_t read_size = 0;
    size_t size;
    size_t end = read_start + length;
    int errsv = 0;

    if ( !length || read_start >= read_max )
    {
        read_start = SIZE_MAX;
        goto clean;
    }

    // adjust end size if it exceeds read_max
    if ( end > read_max )
        end = read_max;

    uint8_t offset_width = countHexWidth64((end>HEX_COL_SIZE)?end-HEX_COL_SIZE:end);
    
    DPrint("  nr_of_parts: 0x%zx\n", nr_of_parts);
    DPrint("  buffer: %p\n", buffer);
    DPrint("  fi: %p\n", fi);
    DPrint("  buffer_size: 0x%zx\n", buffer_size);
    DPrint("  read_start: 0x%zx\n", read_start);
    DPrint("  read_max: 0x%zx\n", read_max);
    DPrint("  end: 0x%zx\n", end);

    for ( p = 0; p < nr_of_parts; p++ )
    {
        read_size = buffer_size;
        DPrint("%zu / %zu\n", (p+1), nr_of_parts);
        DPrint("  read_size: 0x%zx\n", read_size);
        DPrint("  read_start: 0x%zx\n", read_start);

        if ( read_start >= end )
            break;
        if ( read_start + read_size > end )
            read_size = end - read_start;
        if ( read_size == 0 )
            break;
        
        DPrint("  read_size: 0x%zx\n", read_size);
        
        memset(buffer, 0, buffer_size);
        size = readFile(fi, read_start, read_size, buffer, &errsv);
        
        if ( !size )
        {
            EPrint("Reading buffer of bytes failed! (0x%x)\n", errsv);
            read_start = read_max;
            break;
        }

        printPart(buffer, read_start, size, offset_width);

        read_start += read_size;
    }

    if ( read_start >= read_max )
        read_start = SIZE_MAX;

clean:
    FLeave();
    return read_start;
}

void printPart(const uint8_t* buffer, size_t block_start, size_t size, uint8_t offset_width)
{
    FEnter();
    
    if ( g_col_mask == COL_MASK_BYTE_STRING || g_col_mask == (COL_MASK_OFFSET|COL_MASK_BYTE_STRING) )
        printPlainByteString(buffer, size);
    else
        printTripleCols(buffer, size, block_start, offset_width);
    
    FLeave();
}

void printTripleCols(const uint8_t* buffer, size_t size, size_t offset, uint8_t width)
{
    FEnter();

    size_t i;
    uint32_t k = 0;

    uint32_t mask = g_col_mask;
    uint32_t hex_col_size = g_col_sizes.hex;
    uint32_t ascii_col_size = g_col_sizes.ascii;
    uint32_t unicode_col_size = g_col_sizes.unicode;
    uint32_t line_size = g_col_sizes.line;

    DPrint("buffer: %p\n", buffer);
    DPrint("size: 0x%zx\n", size);
    DPrint("offset: 0x%zx\n", offset);
    DPrint("width: 0x%x\n", width);
    DPrint("col_flags: 0x%x\n", g_col_mask);
    DPrint("hex_size: 0x%x\n", g_hex_size);
    DPrint("hex_col_size: 0x%x\n", hex_col_size);
    DPrint("ascii_col_size: 0x%x\n", ascii_col_size);
    DPrint("unicode_col_size: 0x%x\n", unicode_col_size);
    DPrint("line_size: 0x%x\n", line_size);

    for ( i = 0; i < size; i += line_size, offset+=line_size )
    {
        if ( mask & COL_MASK_OFFSET )
        {
            printOffsetCol(offset, width);
            //printOffsetCol(first_line?real_start:offset, width);
        }
        
        if ( mask & COL_MASK_HEX )
        {
            switch ( g_hex_size )
            {
                case 2:  k = printHexCol16(buffer, i, size, hex_col_size); break; // 2,4,... : 16
                case 4:  k = printHexCol32(buffer, i, size, hex_col_size); break; // 4,8,... : 16
                case 8:  k = printHexCol64(buffer, i, size, hex_col_size); break; // 8 | 16
                default: k = printHexCol8(buffer, i, size, hex_col_size); break; // 1 : 16
            }
            
            if ( IS_FLAG_SET(mask, (COL_MASK_ASCII|COL_MASK_UNICODE)) )
            {
                fillHexGap(k, hex_col_size, g_hex_size);
                printf("%c ", COL_SEPARATOR);
            }
        }

        if ( mask & COL_MASK_ASCII )
        {
            printAsciiCol(buffer, i, size, ascii_col_size);
        }
        else if ( mask & COL_MASK_UNICODE )
        {
            printUnicodeCol(buffer, i, size, unicode_col_size);
        }

        printf("\n");
    }

    FLeave();
}

void printOffsetCol(size_t offset, uint32_t width)
{
    printf("%0*zx: ", width, offset);
}

void fillHexGap(uint32_t k, uint32_t col_size, uint32_t hex_size)
{
    uint32_t gap = col_size - k;
    uint32_t gap_units = gap/hex_size;
    //char* gap_filler = "   ";

    switch ( hex_size )
    {
        case 2: gap_units *= 5; break;
        case 4: gap_units *= 9; break;
        case 8: gap_units *= 17; break;
        default: gap_units *= 3; break;
        //case 2: gap_filler = "     "; break;
        //case 4: gap_filler = "         ";  break;
        //case 8: gap_filler = "                 ";   break;
        //default: gap_filler = "   ";   break;
    }

    if ( gap > 0 )
    {
        //printf("%*s", gap_units, "");
        for ( k = 0; k < gap_units; k++ )
        {
            printf(" ");
        }
        //for ( k = 0; k < gap_units; k++ )
        //{
        //    printf("%s", gap_filler);
        //}
    }
}

//void printAsciiCols(const uint8_t* buffer, size_t size, uint32_t col_size)
//{
//    size_t i;
//
//    for ( i = 0; i < size; i += col_size )
//    {
//        printAsciiCol(buffer, i, size, col_size);
//        printf("\n");
//    }
//}

void printAsciiCol(const uint8_t* buffer, size_t i, size_t size, uint32_t col_size)
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

        printAsciiChar(buffer[temp_i]);
    }
}



//void printUnicodeCols(const uint8_t* buffer, size_t size, uint32_t col_size)
//{
//    size_t i;
//
//    for ( i = 0; i < size; i += col_size )
//    {
//        printUnicodeCol(buffer, i, size, col_size);
//        printf("\n");
//    }
//}

void printUnicodeCol(const uint8_t* buffer, size_t i, size_t size, uint32_t col_size)
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

        printUnicodeChar(*(uint16_t*)&buffer[temp_i]);
    }
}



//void printHexCols(const uint8_t* buffer, size_t size)
//{
//    size_t i;
//
//    for ( i = 0; i < size; i += HEX_COL_SIZE )
//    {
//        printHexCol8(buffer, i, size, HEX_COL_SIZE);
//
//        printf("\n");
//    }
//}

//uint32_t printHexCol8(const uint8_t* buffer, size_t i, size_t size, uint32_t col_size)
//{
//    uint32_t k = 0;
//    size_t block_offset;
//    char gap = BLANK_GAP_C;
//    uint32_t gap_ctr = 0;
//    uint32_t start = 0;
//
//    if ( skip_hex_bytes > 0 )
//    {
//        fillHexGap(skip_hex_bytes, col_size, 1);
//        start = skip_hex_bytes;
//        skip_hex_bytes = 0;
//    }
//
//    for ( k = start, gap_ctr=start; k < col_size; k++, gap_ctr++ )
//    {
//        block_offset = i + k;
//        if ( block_offset >= size )
//            break;
//
//        if ( (gap_ctr+1) == col_size/2 )
//            gap = SEPARATOR_GAP_C;
//        else 
//            gap = BLANK_GAP_C;
//        if ( (gap_ctr+1) == col_size )
//            gap_ctr = 0;
//
//        printHexValue(buffer[block_offset], 2, gap);
//    }
//
//    return k;
//}

//uint32_t printHexCol16(const uint8_t* buffer, size_t i, size_t size, uint32_t col_size)
//{
//    uint32_t k = 0;
//    size_t block_offset;
//    char gap = BLANK_GAP_C;
//    uint32_t gap_ctr = 0;
//    uint32_t byte_size = 2;
//    uint32_t str_size = byte_size<<1;
//    uint32_t start = 0;
//    uint32_t separator_id = col_size / (2);
//    //printf("separator_id: 0x%x\n", separator_id);
//
//    if ( skip_hex_bytes > 0 )
//    {
//        uint32_t g = col_size - skip_hex_bytes;
//        fillHexGap(g, col_size, byte_size);
//        start = ALIGN_DOWN_BY(skip_hex_bytes, byte_size);
//        skip_hex_bytes = 0;
//    }
//
//    for ( k = start, gap_ctr=start; k < col_size; k+=byte_size, gap_ctr+=byte_size )
//    {
//        block_offset = i + k;
//        if ( block_offset >= size )
//            break;
//
//        if ( (gap_ctr+byte_size) == separator_id )
//            gap = SEPARATOR_GAP_C;
//        else 
//            gap = BLANK_GAP_C;
//        if ( (gap_ctr+2) == col_size )
//            gap_ctr = 0;
//
//        printHexValue(*(uint16_t*)&buffer[block_offset], str_size, gap);
//    }
//
//    return k;
//}

#define PRINT_HEX_COL_XX(__name__, __byte_size__, __str_size__, __type__) \
uint32_t __name__(const uint8_t* buffer, size_t i, size_t size, uint32_t col_size) \
{ \
    uint32_t k = 0; \
    size_t block_offset; \
    char gap = BLANK_GAP_C; \
    uint32_t gap_ctr = 0; \
    uint32_t start = 0; \
    uint32_t separator_id = col_size / 2; \
     \
    if ( skip_hex_bytes > 0 ) \
    { \
        uint32_t g = col_size - skip_hex_bytes; \
        fillHexGap(g, col_size, __byte_size__); \
        start = ALIGN_DOWN_BY(skip_hex_bytes, __byte_size__); \
        skip_hex_bytes = 0; \
    } \
 \
    for ( k = start, gap_ctr=start; k < col_size; k+=__byte_size__, gap_ctr+=__byte_size__ ) \
    { \
        block_offset = i + k; \
        if ( block_offset >= size ) \
            break; \
         \
        if ( __byte_size__ <= 2 && (gap_ctr+__byte_size__) == separator_id && ((i+k+__byte_size__) < size) ) \
            gap = SEPARATOR_GAP_C; \
        else  \
            gap = BLANK_GAP_C; \
        if ( (gap_ctr+1) == col_size ) \
            gap_ctr = 0; \
         \
        printHexValue(*(__type__*)&buffer[block_offset], __str_size__, gap); \
    } \
     \
    return k; \
}

PRINT_HEX_COL_XX(printHexCol8, 1, 2, uint8_t);
PRINT_HEX_COL_XX(printHexCol16, 2, 4, uint16_t);
PRINT_HEX_COL_XX(printHexCol32, 4, 8, uint32_t);
PRINT_HEX_COL_XX(printHexCol64, 8, 16, uint64_t);

void printPlainByteString(const uint8_t* buffer, size_t size)
{
    FEnter();
    size_t k = 0;
    
    for ( k = 0; k < skip_hex_bytes; k++ )
        printf(BYTE_STR_GAP);

    skip_hex_bytes = 0;

    for ( k = 0; k < size; k++ )
        printf("%02x", buffer[k]);

    FLeave();
}

void printCleanHexValue(const uint64_t v, const uint32_t width, const char gap)
{
    printf("%0*"PRIX64"%c", width, v, gap);
}

void printAnsiFormatedHexValue(const uint64_t v, const uint32_t width, const char gap)
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
void printWinFormatedHexValue(const uint64_t v, const uint32_t width, const char gap)
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
