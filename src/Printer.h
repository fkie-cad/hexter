#ifndef HEXTER_SRC_PRINTER_H
#define HEXTER_SRC_PRINTER_H

void print(uint64_t start, uint8_t skip_bytes);
void printBlockLoop(uint64_t nr_of_parts, unsigned char* block, FILE* fi, uint16_t block_size, uint64_t block_start);
uint64_t printBlock(uint64_t nr_of_parts, unsigned char* block, FILE* fi, uint16_t block_size, uint64_t block_start);
void printLine(const unsigned char* block, uint64_t block_start, uint64_t size, uint8_t offset_width);
void printDoubleCols(unsigned char* block, uint64_t size);
void printTripleCols(unsigned char* block, uint64_t size, uint64_t start, uint8_t width);
void fillGap(uint8_t k);
void printAsciiCols(unsigned char* block, uint64_t size);
void printAsciiCol(unsigned char* block, uint64_t i, uint64_t size, uint8_t col_size);
void printHexCols(unsigned char* block, uint64_t size);
void printOffsetCol(uint64_t offset, uint8_t width);
uint8_t printHexCol(unsigned char* block, uint64_t i, uint64_t size, uint8_t col_size);
void printCleanHexValue(uint8_t b);
void printAnsiFormatedHexValue(uint8_t b);
void printWinFormatedHexValue(uint8_t b);

#endif
