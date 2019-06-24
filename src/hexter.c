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
#include "Writer.h"
#include "utils/Converter.h"
#include "utils/Helper.h"

#define BINARYNAME ("hexter")

uint64_t file_size;
char file_name[PATH_MAX];
uint64_t start;
uint64_t length;
uint8_t clean_printing;
uint8_t skip_bytes;

uint8_t print_col_mask;
uint8_t print_offset_mask;
uint8_t print_hex_mask;
uint8_t print_ascii_mask;

uint8_t insert_f;
uint8_t overwrite_f;
uint8_t find_f;
uint8_t delete_f;
uint8_t continuous_f;

int payload_arg_id;
const char* vs = "1.4.2";

const char FORMAT_ASCII = 'a';
const char FORMAT_BYTE = 'b';
const char FORMAT_WORD = 'w';
const char FORMAT_D_WORD = 'd';
const char FORMAT_Q_WORD = 'q';
const char FORMAT_PLAIN_HEX = 'h';

const char format_types[6] = {'a', 'b', 'w', 'd', 'q', 'h'};
int format_types_ln = 6;

void printUsage();
void initParameters();
void parseArgs(int argc, char** argv);
uint8_t isArgOfType(char* arg, char* type);
uint8_t isFormatArgOfType(char* arg, char* type);
uint8_t hasValue(char* type, int i, int end_i);
void sanitizeParams();
uint32_t parsePayload(const char* arg, const char* value, unsigned char** payload);

// TODO:
// + search option
// + string, byte, (d/q)word,
// + column to show file offset
// + delete option
// - reversed payload, endianess option for hex and word payload
// + interactive more/scroll
// - align offset to 0x10, print spaces to fill col up
// - highlight found part
// - continuouse find typing 'n'
int main(int argc, char** argv)
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
	debug_info("print_col_mask only: %d\n", print_col_mask);
	debug_info("insert: %d\n", insert_f);
	debug_info("overwrite: %d\n", overwrite_f);
	debug_info("find: %d\n", find_f);
	debug_info("delete: %d\n", delete_f);
	debug_info("\n");

	unsigned char* payload = NULL;
	uint32_t payload_ln = 0;

	if ((insert_f || overwrite_f || find_f) && payload_arg_id >= 0 )
	{
		payload_ln = parsePayload(argv[payload_arg_id], argv[payload_arg_id + 1], &payload);
		if ( payload == NULL) exit(0);
	}

	if ( insert_f )
		insert(payload, payload_ln);
	else if ( overwrite_f )
		overwrite(payload, payload_ln);

	file_size = getSize(file_name);
	sanitizeParams();

	if ( find_f )
	{
		start = find(payload, payload_ln, start);
	}
	else if ( delete_f )
	{
		deleteBytes(start, length);
		length = DEFAULT_LENGTH;
	}

	if ( start < UINT64_MAX && !find_f )
		print(start, skip_bytes);

	if ( payload != NULL )
		free(payload);

	return 0;
}

void initParameters()
{
	file_size = 0;
	start = 0;
	length = DEFAULT_LENGTH;
	skip_bytes = 0;

	continuous_f = 1;
	insert_f = 0;
	overwrite_f = 0;
	find_f = 0;
	delete_f = 0;
	payload_arg_id = -1;

	clean_printing = 0;

	print_col_mask = 0;
	print_offset_mask = 4;
	print_hex_mask = 2;
	print_ascii_mask = 1;
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
	printf("Options:\n");
	printf(" * -s:uint64_t Startoffset. Default = 0.\n"
		   " * -l:uint64_t Length of the part to display. Default = 50.\n"
		   " * -a ASCII only print.\n"
		   " * -x HEX only print.\n"
		   " * -p Plain, not styled text output.\n"
		   " * -ix Insert hex byte sequence (destructive!). Where x is an format option.\n"
		   " * -ox Overwrite hex byte sequence (destructive!). Where x is an format option.\n"
		   " * -fx Find hex byte sequence. Where x is an format option.\n"
		   " * * Format options: %c: plain bytes, %c: ascii text, %c: byte, %c: word, %c: double word, %c: quad word.\n"
		   "     Expect for the ascii string, all values have to be passed as hex values.\n"
		   //		   " * -e:uint8_t Endianess of payload (little: 1, big:2). Defaults to 1 = little endian.\n"
		   " * -d Delete -l bytes from offset -s.\n"
		   " * -b Force breaking, not continuous mode.\n"
		   " * -h Print this.\n",
		   FORMAT_PLAIN_HEX, FORMAT_ASCII, FORMAT_BYTE, FORMAT_WORD, FORMAT_D_WORD, FORMAT_Q_WORD
	);
	printf("\n");
	printf("Example: ./%s path/to/a.file -s 100 -l 128 -x\n", BINARYNAME);
	printf("Example: ./%s path/to/a.file -ih dead -s 0x100\n", BINARYNAME);
	printf("Example: ./%s path/to/a.file -oh 0bea -s 0x100\n", BINARYNAME);
	printf("Example: ./%s path/to/a.file -fh f001 -s 0x100\n", BINARYNAME);
	printf("Example: ./%s path/to/a.file -d -s 0x100 -l 0x8\n", BINARYNAME);
}

void parseArgs(int argc, char** argv)
{
	int start_i = 1;
	int end_i = argc - 1;
	int i, s;
	uint8_t length_found = 0;

	if ( isArgOfType(argv[1], "-h"))
	{
		printHelp();
		exit(0);
	}

	if ( argv[1][0] != '-' )
	{
		expandFilePath(argv[1], file_name);
		start_i = 2;
		end_i = argc;
	}

	for ( i = start_i; i < end_i; i++ )
	{
		if ( argv[i][0] != '-' )
			break;

		if ( isArgOfType(argv[i], "-x") )
		{
			print_col_mask = print_col_mask | print_hex_mask;
		}
		else if ( isArgOfType(argv[i], "-a") )
		{
			print_col_mask = print_col_mask | print_ascii_mask;
		}
		else if ( isArgOfType(argv[i], "-p") )
		{
			clean_printing = 1;
		}
		else if ( isArgOfType(argv[i], "-d") )
		{
			delete_f = 1;
		}
		else if ( isArgOfType(argv[i], "-b") )
		{
			continuous_f = 0;
		}
		else if ( isArgOfType(argv[i], "-s") )
		{
			if ( hasValue("-s", i, end_i) )
			{
				s = parseUint64Auto(argv[i + 1], &start);
				if ( s != 0 )
				{
					printf("INFO: Could not parse start. Setting it to %u!\n", 0);
					start = 0x00;
				}
				i++;
			}
		}
		else if ( isArgOfType(argv[i], "-l") )
		{
			if ( hasValue("-l", i, end_i) )
			{
				s = parseUint64Auto(argv[i + 1], &length);
				if ( s != 0 )
				{
					printf("INFO: Could not parse length. Setting it to %u!\n", DEFAULT_LENGTH);
					length = DEFAULT_LENGTH;
				}
				else
					length_found = 1;
				i++;
			}
		}
		else if ( isFormatArgOfType(argv[i], "-i") )
		{
			if ( hasValue("-i", i, end_i))
			{
				insert_f = 1;
				payload_arg_id = i;
				i++;
			}
		}
		else if ( isFormatArgOfType(argv[i], "-o") )
		{
			if ( hasValue("-o", i, end_i))
			{
				overwrite_f = 1;
				payload_arg_id = i;
				i++;
			}
		}
		else if ( isFormatArgOfType(argv[i], "-f") )
		{
			if ( hasValue("-f", i, end_i))
			{
				find_f = 1;
				payload_arg_id = i;
				i++;
			}
		}
		else
		{
			printf("INFO: Unknown arg type \"%s\"\n", argv[i]);
		}
	}

	if ((find_f + overwrite_f + insert_f + delete_f) > 1 )
	{
		printf("ERROR: overwrite, insert, delete and find have to be used exclusively!\n");
		exit(0);
	}

	if ( delete_f && !length_found )
	{
		printf("ERROR: could not parse length of part to delete!\n");
		exit(0);
	}

	if ( start_i == 1 )
		expandFilePath(argv[i], file_name);
}

uint8_t isArgOfType(char* arg, char* type)
{
	int type_ln = strnlen(type, 10);
	return strnlen(arg, 10) == type_ln && strncmp(arg, type, type_ln) == 0;
}

uint8_t isFormatArgOfType(char* arg, char* type)
{
	int i, j;
	int arg_ln = strnlen(arg, 10);
	int type_ln = strnlen(type, 10);

	if ( arg_ln <= type_ln )
		return 0;

	for ( i = 0; i < type_ln; i++ )
		if ( arg[i] != type[i] )
			return 0;

	j = i;
	for ( i = 0; i < format_types_ln; i++ )
		if ( format_types[i] == arg[j] )
			return 1;

	return 0;
}

uint8_t hasValue(char* type, int i, int end_i)
{
	if ( i >= end_i - 1 )
	{
		printf("INFO: Arg \"%s\" has no value! Skipped!\n", type);
		return 0;
	}

	return 1;
}

void sanitizeParams()
{
	uint8_t col_size;

	if ( print_col_mask == 0 )
		print_col_mask = (print_offset_mask | print_ascii_mask | print_hex_mask);

	if ( insert_f || overwrite_f || delete_f )
		continuous_f = 0;

	col_size = getColSize();

	if ( continuous_f )
	{
		if ( length % col_size != 0 )
		{
			length = length + col_size - (length % col_size);
		}
	}

	uint8_t info_line_break = 0;
	if ( start > file_size )
	{
		fprintf(stderr, "Info: Start offset %lu is greater the the file_size %lu!\nSetting to 0!", start, file_size);
		start = 0;
		info_line_break = 1;
	}
	if ( start + length > file_size )
	{
		fprintf(stdout,
				"Info: Start offset %lu plus length %lu is greater the the file size %lu\nPrinting only to file size.\n",
				start, length, file_size);
		length = file_size - start;
		info_line_break = 1;
	}
	else if ( length == 0 )
	{
		fprintf(stdout, "Info: Length is 0. Setting to %u!\n", DEFAULT_LENGTH);
		length = DEFAULT_LENGTH;
		info_line_break = 1;
	}

	if ( info_line_break )
		printf("\n");


	start = normalizeOffset(start, &skip_bytes);
	if ( !continuous_f )
		length += skip_bytes;
}

uint32_t parsePayload(const char* arg, const char* value, unsigned char** payload)
{
	uint32_t ln = 0;
	char format = arg[2];

	if ( strnlen(value, MAX_PAYLOAD_LN) < 1 )
		return 0;

	if ( format == FORMAT_BYTE )
		ln = payloadParseByte(value, payload);
	else if ( format == FORMAT_WORD )
		ln = payloadParseWord(value, payload);
	else if ( format == FORMAT_D_WORD )
		ln = payloadParseDWord(value, payload);
	else if ( format == FORMAT_Q_WORD )
		ln = payloadParseQWord(value, payload);
	else if ( format == FORMAT_ASCII )
		ln = payloadParseString(value, payload);
//	else if ( format == 'r' )
//		ln = payloadParseReversedPlainBytes(arg, payload);
	else if ( format == FORMAT_PLAIN_HEX )
		ln = payloadParsePlainBytes(value, payload);
	else
	{
		printf("ERROR: No format specifier found in %s!\n", arg);
		ln = 0;
	}

//	ln = payloadParsePlainBytes(arg, payload);

	return ln;
}
