#ifndef HEXTER_SRC_PRINTER_H
#define HEXTER_SRC_PRINTER_H

#define HIGHLIGHT_HEX_STYLE "\033[1;42m"
#define POS_HEX_STYLE "\033[1m"
#define LIGHT_STYLE "\033[38;2;150;150;150m"

void print(size_t start, uint8_t skip_bytes, unsigned char* needle, uint32_t needle_ln);
void setPrintingStyle();
void Printer_cleanUp(unsigned char* block, FILE* fi);
void printBlockLoop(size_t nr_of_parts, unsigned char* block, FILE* fi, uint16_t block_size, size_t block_start, size_t block_max);
size_t printBlock(size_t nr_of_parts, unsigned char* block, FILE* fi, uint16_t block_size, size_t read_start, size_t read_max);
void printLine(const unsigned char* block, size_t block_start, size_t size, uint8_t offset_width);
void printDoubleCols(const unsigned char* block, size_t size);
void printTripleCols(const unsigned char* block, size_t size, size_t offset, uint8_t width);
void fillGap(uint8_t k);
void printAsciiCols(const unsigned char* block, size_t size);
void printAsciiCol(const unsigned char* block, size_t i, size_t size, uint8_t col_size);
void printHexCols(const unsigned char* block, size_t size);
void printOffsetCol(size_t offset, uint8_t width);
uint8_t printHexCol(const unsigned char* block, size_t i, size_t size, uint8_t col_size);
void printCleanHexValue(uint8_t b);
void printAnsiFormatedHexValue(const unsigned char b);
#ifdef _WIN32
void printWinFormatedHexValue(const unsigned char b);
#endif
void Printer_setHighlightBytes(uint32_t v);
void Printer_setSkipBytes(uint8_t skip_bytes);
void Printer_setHighlightWait(uint32_t v);

#endif
