#ifndef HEXTER_SRC_PRINTER_H
#define HEXTER_SRC_PRINTER_H

void print(uint64_t start, uint8_t skip_bytes, unsigned char* needle, uint32_t needle_ln);
void setPrintingStyle();
void Printer_cleanUp(unsigned char* block, FILE* fi);
void printBlockLoop(uint64_t nr_of_parts, unsigned char* block, FILE* fi, uint16_t block_size, uint64_t block_start);
uint64_t printBlock(uint64_t nr_of_parts, unsigned char* block, FILE* fi, uint16_t block_size, uint64_t block_start);
void printLine(const unsigned char* block, uint64_t block_start, uint64_t size, uint8_t offset_width);
void printDoubleCols(const unsigned char* block, uint64_t size);
void printTripleCols(const unsigned char* block, uint64_t size, uint64_t offset, uint8_t width);
void fillGap(uint8_t k);
void printAsciiCols(const unsigned char* block, uint64_t size);
void printAsciiCol(const unsigned char* block, uint64_t i, uint64_t size, uint8_t col_size);
void printHexCols(const unsigned char* block, uint64_t size);
void printOffsetCol(uint64_t offset, uint8_t width);
uint8_t printHexCol(const unsigned char* block, uint64_t i, uint64_t size, uint8_t col_size);
void printCleanHexValue(uint8_t b);
void printAnsiFormatedHexValue(const unsigned char b);
void setAnsiFormat(char* format);
void resetAnsiFormat();
#ifdef _WIN32
void printWinFormatedHexValue(const unsigned char b);
#endif
void Printer_setHiglightBytes(int16_t v);
void Printer_setSkipBytes(uint8_t skip_bytes);
void Printer_setHiglightWait(int16_t v);

#endif
