#ifndef HEXTER_SRC_PRINTER_H
#define HEXTER_SRC_PRINTER_H

void print();
void printDoubleCols(unsigned char* block, uint64_t size);
void printAsciiCols(unsigned char* block, uint64_t size);
void printHexCols(unsigned char* block, uint64_t size);

#endif
