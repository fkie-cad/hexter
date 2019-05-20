#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "Globals.h"
#include "utils/common_fileio.h"
#include "Finder.h"
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
uint8_t find_f;

int payload_arg_id;
const char* vs = "1.3.0";

void printUsage();
void initParameters();
void parseArgs(int argc, char **argv);
uint8_t isArgOfType(char* arg, char* type);
uint8_t hasValue(char* type, int i, int end_i);
void sanitizeOffsets();
uint32_t parsePayload(const char* arg, unsigned char** payload);

// TODO:
// - search option
// - delete option
// - interactive more/scroll
// - string, byte, (d/q)word, reversed payload
// - endianess option for byte and word payload

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

	debug_info("file_name: %s\n", file_name);
	debug_info("file_size: %lu\n", file_size);
	debug_info("start: %lu\n", start);
	debug_info("length: %lu\n", length);
	debug_info("ascii only: %d\n", ascii_only);
	debug_info("hex only: %d\n", hex_only);
	debug_info("insert: %d\n", insert_f);
	debug_info("overwrite: %d\n", overwrite_f);
	debug_info("find: %d\n", find_f);
	debug_info("\n");

	unsigned char* payload = NULL;
	uint32_t payload_ln = 0;

	if ( (insert_f || overwrite_f || find_f) && payload_arg_id > -1 )
	{
		payload_ln = parsePayload(argv[payload_arg_id], &payload);
		if ( payload == NULL ) exit(0);
	}

	if ( insert_f )
		insert(payload, payload_ln);
	if ( overwrite_f )
		overwrite(payload, payload_ln);

	file_size = getSize(file_name);
	sanitizeOffsets();

	if ( find_f )
	{
		start = find(payload, payload_ln);
		if ( start == UINT64_MAX )
			printf("Pattern not found!\n");
	}

	if ( start < UINT64_MAX )
		print();

	if ( payload != NULL )
		free(payload);

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
	find_f = 0;
	payload_arg_id = -1;
}

void printUsage()
{
	printf("Usage: ./%s filename [options]\n", BINARYNAME);
	printf("Usage: ./%s [options] filename\n", BINARYNAME);
	printf("Version: %s\n", vs);
}

void printHelp()
{
	printf("Usage: ./%s filename [options]\n", BINARYNAME);
	printf("Usage: ./%s [options] filename\n", BINARYNAME);
	printf("Version: %s\n", vs);
	printf(" * -s:uint64_t Startoffset. Default = 0.\n"
		   " * -l:uint64_t Length of the part to display. Default = 50.\n"
		   " * -a ASCII only print.\n"
		   " * -x HEX only print.\n"
		   " * -c Clean output (no text formatin in the console).\n"
		   " * -i Insert hex byte sequence (destructive!).\n"
		   " * -o Overwrite hex byte sequence (destructive!).\n"
		   " * -f Find hex byte sequence.\n"
		   " * -h Print this.\n"
//		   " * -e:uint8_t Endianess of payload (little: 1, big:2). Defaults to 1 = little endian.\n"
		   );
	printf("\n");
	printf("Example: ./%s path/to/a.file -s 100 -l 128 -x\n",BINARYNAME);
	printf("Example: ./%s path/to/a.file -i dead -s 0x100\n",BINARYNAME);
	printf("Example: ./%s path/to/a.file -o 0bea -s 0x100\n",BINARYNAME);
	printf("Example: ./%s path/to/a.file -f f001 -s 0x100\n",BINARYNAME);
}

void parseArgs(int argc, char **argv)
{
	int start_i = 1;
	int end_i = argc - 1;
	int i, s;
	uint8_t arg_found = 0;

	if ( isArgOfType(argv[1], "-h") )
	{
		printHelp();
		exit(0);
	}

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
				s = parseUint64Auto(argv[i + 1], &start);
				if ( s != 0 )
				{
					printf("INFO: Could not parse start, setting to %u!\n", 0);
					start = 0x00;
				}
				i++;
			}
		}
		if ( arg_found == 0 && isArgOfType(argv[i], "-l") )
		{
			arg_found = 1;
			if ( hasValue("-l", i, end_i) )
			{
				s = parseUint64Auto(argv[i + 1], &length);
				if ( s != 0 )
				{
					printf("INFO: Could not parse length, setting to %u!\n", DEFAULT_LENGTH);
					length = DEFAULT_LENGTH;
				}
				i++;
			}
		}
		if ( arg_found == 0 && isArgOfType(argv[i], "-i") )
		{
			arg_found = 1;
			if ( hasValue("-i", i, end_i) )
			{
				insert_f = 1;
				payload_arg_id = i+1;
				i++;
			}
		}
		if ( arg_found == 0 && isArgOfType(argv[i], "-o") )
		{
			arg_found = 1;
			if ( hasValue("-o", i, end_i) )
			{
				overwrite_f = 1;
				payload_arg_id = i+1;
				i++;
			}
		}
		if ( arg_found == 0 && isArgOfType(argv[i], "-f") )
		{
			arg_found = 1;
			if ( hasValue("-f", i, end_i) )
			{
				find_f = 1;
				payload_arg_id = i+1;
				i++;
			}
		}

		if ( !arg_found )
		{
			printf("INFO: Unknown arg type \"%s\"\n", argv[i]);
		}
	}

	if ( (find_f + overwrite_f + insert_f) > 1 )
	{
		printf("ERROR: overwrite, insert and find have to be used exclusively!\n");
		exit(0);
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

void sanitizeOffsets()
{
	uint8_t info_line_break = 0;
	if ( start > file_size )
	{
		fprintf(stderr, "Info: Start offset %lu is greater the the file_size %lu!\nSetting to 0!", start, file_size);
		start = 0;
		info_line_break = 1;
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

uint32_t parsePayload(const char* arg, unsigned char** payload)
{
	uint32_t ln;

	if ( strnlen(arg, MAX_PAYLOAD_LN) < 2 )
		return 0;

//	if ( arg[0] == 'b' )
//		ln = payloadParseByte(arg, payload);
//	else if ( arg[0] == 'w' )
//		ln = payloadParseWord(arg, payload);
//	else if ( arg[0] == 'd' && arg[1] == 'w' )
//		ln = payloadParseDoubleWord(arg, payload);
//	else if ( arg[0] == 'q' && arg[1] == 'w' )
//		ln = payloadParseQuadWord(arg, payload);
//	else if ( arg[0] == '"' )
//		ln = payloadParseString(arg, payload);
//	else if ( arg[0] == 'r' )
//		ln = payloadParseReversedPlainBytes(arg, payload);
//	else
//		ln = payloadParsePlainBytes(arg, payload);

	ln = payloadParsePlainBytes(arg, payload);

	return ln;
}
