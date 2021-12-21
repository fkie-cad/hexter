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

size_t printBlock(
    size_t nr_of_parts,
    uint8_t* block,
    FILE* fi,
    uint16_t block_size,
    size_t read_start,
    size_t read_max
);

void printLine(
    const uint8_t* block, 
    size_t block_start, 
    size_t size, 
    uint8_t offset_width
);

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
