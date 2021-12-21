#ifndef HEXTER_SRC_PRINTER_H
#define HEXTER_SRC_PRINTER_H

#define HIGHLIGHT_HEX_STYLE "\033[1;42m"
#define POS_HEX_STYLE "\033[1m"
#define LIGHT_STYLE "\033[38;2;150;150;150m"

void print(
    size_t start, 
    uint8_t skip_bytes, 
    uint8_t* needle, 
    uint32_t needle_ln
);

void setPrintingStyle();

void Printer_cleanUp(
    uint8_t* block, 
    FILE* fi
);

static void printBlockLoop(size_t nr_of_parts, uint8_t* block, FILE* fi, uint16_t block_size, size_t block_start, size_t block_max);
static size_t printBlock(size_t nr_of_parts, uint8_t* block, FILE* fi, uint16_t block_size, size_t read_start, size_t read_max);

void printLine(
    const uint8_t* block, 
    size_t block_start, 
    size_t size, 
    uint8_t offset_width
);

static void printDoubleCols(const uint8_t* block, size_t size, void (*printCol)(const uint8_t*, size_t, size_t, uint16_t));

static void printTripleCols(const uint8_t* block, size_t size, size_t offset, uint8_t width, void (*printCol)(const uint8_t*, size_t, size_t, uint16_t));

static void fillGap(uint8_t k);

static void printAsciiCols(const uint8_t* block, size_t size, uint16_t col_size);
static void printAsciiCol(const uint8_t* block, size_t i, size_t size, uint16_t col_size);

static void printUnicodeCols(const uint8_t* block, size_t size, uint16_t col_size);
static void printUnicodeCol(const uint8_t* block, size_t i, size_t size, uint16_t col_size);

static void printHexCols(const uint8_t* block, size_t size);
static uint8_t printHexCol(const uint8_t* block, size_t i, size_t size, uint8_t col_size);

static void printOffsetCol(size_t offset, uint8_t width);
static void printCleanHexValue(uint8_t b);
static void printAnsiFormatedHexValue(const unsigned char b);
#ifdef _WIN32
static void printWinFormatedHexValue(const unsigned char b);
#endif

void Printer_setHighlightBytes(
    uint32_t v
);

void Printer_setSkipBytes(
    uint8_t skip_bytes
);

void Printer_setHighlightWait(
    uint32_t v
);

#endif
