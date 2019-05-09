#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "Globals.h"
#include "utils/common_fileio.h"
#include "Printer.h"
#include "Payloader.h"
#include "utils/Converter.h"
#include "utils/Helper.h"

#define BINARYNAME ("hexter")

void printUsage();
void initParameters();
void parseArgs(int argc, char **argv);
uint8_t isStandaloneArg(char* arg, char* expected);
uint8_t isValueArg(char* arg, char* expected, int i, int argc);
void sanitizeOffsets();
unsigned char* parsePayload(const char* arg);

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printUsage();
        return -1;
    }

	initParameters();
	parseArgs(argc, argv);

	file_size = getSize(file_name);
    if ( file_size == 0 ) return 0;

    sanitizeOffsets();

	debug_info("file_name: %s\n", file_name);
	debug_info("file_size: %lu\n", file_size);
	debug_info("start: %lu\n", start);
	debug_info("length: %lu\n", length);
	debug_info("ascii only: %d\n", ascii_only);
	debug_info("hex only: %d\n", hex_only);
	debug_info("insert: %d\n", insert);
	debug_info("overwrite: %d\n", overwrite);
	debug_info("\n");

	if ( insert_f )
		insert();
	if ( overwrite_f )
		overwrite();

	print();

	return 0;
}

void initParameters()
{
	file_size = 0;
	start = 0;
	length = DEFAULT_LENGTH;
	ascii_only = 0;
	hex_only = 0;
	clean_printing = 0;
	insert_f = 0;
	overwrite_f = 0;
	payload = NULL;
}

void printUsage()
{
	printf("Usage: ./%s filename [options]\n",BINARYNAME);
	printf("Version: 1.0.1\n",BINARYNAME);
	printf(" * -s:uint64_t Startoffset. Default = 0.\n"
		   " * -l:uint64_t Length of the part to display. Default = 50.\n"
		   " * -a ASCII only print.\n"
		   " * -x HEX only print.\n"
		   " * -c clean output (no text formatin in the console).\n"
		   " * -i insert hex byte sequence (only works with a given -s offset)\n"
		   " * -o overwrite hex byte sequence (only works with a given -s offset)\n"
		   );
	printf("Example: ./%s path/to/a.file -s 100 -l 128 -x\n",BINARYNAME);
	printf("Example: ./%s path/to/a.file -i dead -s 0x200\n",BINARYNAME);
	printf("Example: ./%s path/to/a.file -o beef -s 0x200\n",BINARYNAME);
}

void parseArgs(int argc, char **argv)
{
	int i;

	expandFilePath(argv[1], file_name);

	if ( argc <= 2 )
		return;

	for ( i = 2; i < argc; i++ )
	{
		if ( isValueArg(argv[i], "-s", i, argc) )
			start = parseUint64(argv[i + 1]);
		if ( isValueArg(argv[i], "-l", i, argc) )
			length = parseUint64(argv[i + 1]);
		if ( isStandaloneArg(argv[i], "-a") )
			ascii_only = 1;
		if ( isStandaloneArg(argv[i], "-x") )
			hex_only = 1;
		if ( isStandaloneArg(argv[i], "-c") )
			clean_printing = 1;
		if ( isValueArg(argv[i], "-i", i, argc) )
		{
			insert_f = 1;
			payload = parsePayload(argv[i + 1]);
			if ( payload == NULL ) exit(0);
		}
		if ( isValueArg(argv[i], "-o", i, argc) )
		{
			overwrite_f = 1;
			payload = parsePayload(argv[i + 1]);
			if ( payload == NULL ) exit(0);
		}
	}
}

uint8_t isStandaloneArg(char* arg, char* expected)
{
	return strncmp(arg, expected, 2) == 0;
}

uint8_t isValueArg(char* arg, char* expected, int i, int argc)
{
	return strncmp(arg, expected, 2) == 0 && i < argc - 1 ;
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
		fprintf(stdout, "Info: Start offset %lu plus length %lu is greater the the file size %lu\nPrinting only to file size.\n",
				start, length, file_size);
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

unsigned char* parsePayload(const char* arg)
{
	payload_ln = strnlen(arg, MAX_PAYLOAD_LN);
	uint32_t i, j;
	unsigned char* p = (unsigned char*) malloc(payload_ln/2);
	char byte[3] = {0};

	if ( payload_ln % 2 != 0 )
	{
		printf("Error: Payload is not byte aligned!");
		return NULL;
	}

	for ( i = 0, j=0; i < payload_ln; i += 2 )
	{
		byte[0] = arg[i];
		byte[1] = arg[i+1];

		p[j++] = parseUint8(byte);
	}

	payload_ln = payload_ln / 2;

	return p;
}