#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "Globals.h"
#include "common_fileio.h"
#include "Printer.h"
#include "utils/Converter.h"
#include "utils/Helper.h"

#define BINARYNAME ("hexter")

void printUsage();
void parseArgs(int argc, char **argv);
void sanitizeOffsets();

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printUsage();
        return -1;
    }

    file_size = 0;
	start = 0;
	length = DEFAULT_LENGTH;
	ascii_only = 0;
	hex_only = 0;

	parseArgs(argc, argv);

	file_size = getSize(file_name);
    if ( file_size == 0 ) return 0;

    sanitizeOffsets();

	debug_info("file_name: %s\n", file_name);
	debug_info("file_size: %lu b\n", file_size);
	debug_info("start: %lu\n", start);
	debug_info("length: %lu\n", length);
	debug_info("ascii only: %d\n", ascii_only);
	debug_info("hex only: %d\n", hex_only);
	debug_info("\n");

	print();

	return 0;
}

void printUsage()
{
	printf("Usage: ./%s filename [options]\nVersion 1.0.0\n",BINARYNAME);
	printf(" * -s:uint64_t Startoffset. Default = 0.\n"
		   " * -l:uint64_t Length of the part to display. Default = 50.\n"
		   " * -a ASCII only print.\n"
		   " * -x HEX only print.\n");
	printf("Example: ./%s path/to/a.file -s 100 -l 128 -x\n",BINARYNAME);
}

void parseArgs(int argc, char **argv)
{
	int i;

	expandFilePath(argv[1], file_name);

	if ( argc <= 2 )
		return;

	for ( i = 2; i < argc; i++ )
	{
		if ( strncmp(argv[i], "-s", 2) == 0 && i < argc - 1 )
			start = parseUint64(argv[i + 1]);
		if ( strncmp(argv[i], "-l", 2) == 0 && i < argc - 1 )
			length = parseUint64(argv[i + 1]);
		if ( strncmp(argv[i], "-a", 2) == 0 )
			ascii_only = 1;
		if ( strncmp(argv[i], "-x", 2) == 0 )
			hex_only = 1;
	}
}

void sanitizeOffsets()
{
	uint8_t info_line_break = 0;
	if ( start > file_size )
	{
		fprintf(stderr, "Error: Start offset %lu is greater the the file_size %lu\n", start, file_size);
		exit(0);
	}
	if ( start + length > file_size )
	{
		fprintf(stdout, "Info: Start offset %lu plus length %lu is greater the the file size %lu\nPrinting only to file size.\n", start, length, file_size);
		length = file_size - start;
		info_line_break = 1;
	}
	else if ( length == 0 )
	{
		fprintf(stdout, "Info: Length is 0. Setting to 0x50!\n");
		length = DEFAULT_LENGTH;
		info_line_break = 1;
	}

	if ( info_line_break )
		printf("\n");
}
