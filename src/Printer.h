#ifndef HEXTER_SRC_PRINTER_H
#define HEXTER_SRC_PRINTER_H

void print();
void printDoubleCols(unsigned char* block, uint64_t size);
void fillGap(uint64_t k);
void printAsciiCols(unsigned char* block, uint64_t size);
void printAsciiCol(unsigned char* block, uint64_t i, uint64_t size, uint8_t col_size);
void printHexCols(unsigned char* block, uint64_t size);
uint64_t printHexCol(unsigned char* block, uint64_t i, uint64_t size, uint8_t col_size);
void printCleanHexValue(uint8_t b);
void printFormatedHexValue(uint8_t b);

#endif
