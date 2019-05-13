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

uint64_t file_size;
char file_name[PATH_MAX];
uint64_t start;
uint64_t length;
uint8_t ascii_only;
uint8_t hex_only;
uint8_t clean_printing;
uint8_t insert_f;
uint8_t overwrite_f;
unsigned char* payload;
uint32_t payload_ln;

void printUsage();
void initParameters();
void parseArgs(int argc, char **argv);
uint8_t isArgOfType(char* arg, char* type);
uint8_t hasValue(char* type, int i, int end_i);
void sanitizeOffsets();
unsigned char* parsePayload(const char* arg);

int main(int argc, char **argv)
{
    if ( argc < 2 )
    {
        printUsage();
        return -1;
    }

	initParameters();
	parseArgs(argc, argv);

	file_size = getSize(file_name);
    if ( file_size == 0 ) return 0;

// sanitizeOffsets();

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

	file_size = getSize(file_name);
	sanitizeOffsets();

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
	printf("Usage: ./%s filename [options]\n", BINARYNAME);
	printf("Usage: ./%s [options] filename\n", BINARYNAME);
	printf("Version: 1.2.0\n", BINARYNAME);
	printf(" * -s:uint64_t Startoffset. Default = 0.\n"
		   " * -l:uint64_t Length of the part to display. Default = 50.\n"
		   " * -a ASCII only print.\n"
		   " * -x HEX only print.\n"
		   " * -c clean output (no text formatin in the console).\n"
		   " * -i insert hex byte sequence (destructive!)\n"
		   " * -o overwrite hex byte sequence (destructive!)\n"
		   );
	printf("\n");
	printf("Example: ./%s path/to/a.file -s 100 -l 128 -x\n",BINARYNAME);
	printf("Example: ./%s path/to/a.file -i dead -s 0x100\n",BINARYNAME);
	printf("Example: ./%s path/to/a.file -o 0bea -s 0x100\n",BINARYNAME);
}

void parseArgs(int argc, char **argv)
{
	int start_i = 1;
	int end_i = argc - 1;
	int i;
	uint8_t arg_found = 0;

	if ( argv[1][0] != '-' )
	{
		expandFilePath(argv[1], file_name);
		start_i  = 2;
		end_i = argc;
	}

	for ( i = start_i; i < end_i; i++ )
	{
		if ( argv[i][0] != '-' )
			break;

		arg_found = 0;

		if ( isArgOfType(argv[i], "-x") )
		{
			hex_only = 1;
			arg_found = 1;
		}
		if ( arg_found == 0 && isArgOfType(argv[i], "-a") )
		{
			ascii_only = 1;
			arg_found = 1;
		}
		if ( arg_found == 0 && isArgOfType(argv[i], "-c") )
		{
			clean_printing = 1;
			arg_found = 1;
		}
		if ( arg_found == 0 && isArgOfType(argv[i], "-s") )
		{
			arg_found = 1;
			if ( hasValue("-s", i, end_i) )
			{
				start = parseUint64(argv[i + 1]);
				i++;
			}
		}
		if ( arg_found == 0 && isArgOfType(argv[i], "-l") )
		{
			arg_found = 1;
			if ( hasValue("-l", i, end_i) )
			{
				length = parseUint64(argv[i + 1]);
				i++;
			}
		}
		if ( arg_found == 0 && isArgOfType(argv[i], "-i") )
		{
			arg_found = 1;
			if ( hasValue("-i", i, end_i) )
			{
				insert_f = 1;
				payload = parsePayload(argv[i + 1]);
				if ( payload == NULL ) exit(0);
				i++;
			}
		}
		if ( arg_found == 0 && isArgOfType(argv[i], "-o") )
		{
			arg_found = 1;
			if ( hasValue("-o", i, end_i) )
			{
				overwrite_f = 1;
				payload = parsePayload(argv[i + 1]);
				if ( payload == NULL ) exit(0);
				i++;
			}
		}

		if ( !arg_found )
		{
			printf("INFO: Unknown arg type \"%s\"\n", argv[i]);
		}
	}

	if ( start_i == 1 )
		expandFilePath(argv[i], file_name);
}

uint8_t isArgOfType(char* arg, char* type)
{
	int type_ln;

	type_ln = strlen(type);

	return strnlen(arg, 10) == type_ln && strncmp(arg, type, type_ln) == 0;
}

uint8_t hasValue(char* type, int i, int end_i)
{
	if ( i >= end_i -1 )
	{
		printf("INFO: Arg \"%s\" has no value! Skipped!\n", type);
		return 0;
	}

	return 1;
}

uint8_t isStandaloneArg(char* arg, char* expected)
{
	int expected_ln = strlen(expected);
	return strnlen(arg, 10) == expected_ln && strncmp(arg, expected, 2) == 0;
}

uint8_t isValueArg(char* arg, char* expected, int i, int argc)
{
	int expected_ln = strlen(expected);
	return strnlen(arg, 10) == expected_ln && strncmp(arg, expected, 2) == 0 && i < argc - 1 ;
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