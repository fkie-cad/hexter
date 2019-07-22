#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "Globals.h"
#include "utils/common_fileio.h"
#include "Finder.h"
#include "Printer.h"
#include "Writer.h"
#include "utils/Converter.h"
#include "utils/Helper.h"
#if defined(__linux__) || defined(__linux) || defined(linux)
	#include "ProcessHandlerLinux.h"
#elif defined(_WIN32)
	#include <process.h>
	#include "ProcessHandlerWin.h"
#endif

#define BINARYNAME ("hexter")

uint64_t file_size;
char file_path[PATH_MAX];
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

bool list_process_memory_f;
bool list_process_modules_f;
bool list_process_threads_f;
int list_process_heaps_f;

const uint8_t TYPE_FILE = 1;
const uint8_t TYPE_PID = 2;
uint8_t type;

int payload_arg_id;
const char* vs = "1.5.0";

#define FORMAT_ASCII 'a'
#define FORMAT_BYTE 'b'
#define FORMAT_WORD 'w'
#define FORMAT_D_WORD 'd'
#define FORMAT_Q_WORD 'q'
#define FORMAT_PLAIN_HEX 'h'

const char format_types[6] = { FORMAT_ASCII, FORMAT_BYTE, FORMAT_WORD, FORMAT_D_WORD, FORMAT_Q_WORD, FORMAT_PLAIN_HEX };
int format_types_ln = 6;

void printUsage();
void initParameters();
void parseArgs(int argc, char** argv);
uint8_t isArgOfType(char* arg, char* type);
uint8_t isFormatArgOfType(char* arg, char* type);
uint8_t hasValue(char* type, int i, int end_i);
void sanitizeParams(uint32_t pid);
uint32_t parsePayload(const char* arg, const char* value, unsigned char** payload);
uint8_t parseType(const char* arg);

uint8_t keepStartInFile();
uint8_t keepLengthInFile();

// TODO:
// - highlight found part
// + continuouse find typing 'n'
// - reversed payload, endianess option for hex and word payload
// + align offset to 0x10, print spaces to fill col up
// - view processes
//   - windows
//   - linux
int main(int argc, char** argv)
{
	char* file_name = NULL;
	uint32_t pid = 0;
	unsigned char* payload = NULL;
	uint32_t payload_ln = 0;

	if ( argc < 2 )
	{
		printUsage();
		return -1;
	}

	initParameters();
	parseArgs(argc, argv);

	if ( type == TYPE_FILE )
	{
		file_size = getSize(file_path);
		if ( file_size == 0 ) return 0;
	}
	else if ( type == TYPE_PID )
	{
		int s = parseUint32(file_path, &pid, 10);
		if ( pid == 0 )
#if defined(__linux__) || defined(__linux) || defined(linux)
			pid = getpid();
#elif defined(_WIN32)
			pid = _getpid();
#endif
		file_size = getSizeOfProcess(pid);
		if ( file_size == 0 ) return 0;
	}
	
	debug_info("file_path: %s\n", file_path);
	debug_info("file_size: %lu\n", file_size);
	debug_info("start: %lu\n", start);
	debug_info("length: %lu\n", length);
	debug_info("print_col_mask only: %d\n", print_col_mask);
	debug_info("insert: %d\n", insert_f);
	debug_info("overwrite: %d\n", overwrite_f);
	debug_info("find: %d\n", find_f);
	debug_info("delete: %d\n", delete_f);
	debug_info("\n");

	if ( (insert_f || overwrite_f || find_f) && payload_arg_id >= 0 )
	{
		payload_ln = parsePayload(argv[payload_arg_id], argv[payload_arg_id + 1], &payload);
		if ( payload == NULL) exit(0);
	}

	if ( insert_f )
		insert(payload, payload_ln, start);
	else if ( overwrite_f && type == TYPE_FILE )
		overwrite(payload, payload_ln, start);
	else if ( overwrite_f && type == TYPE_PID )
		writeProcessMemory(pid, payload, payload_ln, start);

	sanitizeParams(pid);

	if ( delete_f )
	{
		deleteBytes(start, length);
		length = DEFAULT_LENGTH;
	}

	setPrintingStyle();
	if ( type == TYPE_FILE )
	{
		getFileNameL(file_path, &file_name);
		printf("file: %s\n", file_name);
		print(start, skip_bytes, payload, payload_ln);
	}
	else if ( type == TYPE_PID )
	{
		printf("pid: %u\n", pid);
		if ( list_process_memory_f )
			listProcessMemory(pid);
		if ( list_process_modules_f )
			listProcessModules(pid);
		if ( list_process_threads_f )
			listProcessThreads(pid);
		if ( list_process_heaps_f )
			listProcessHeaps(pid, list_process_heaps_f);
		printProcessRegions(pid, start, skip_bytes, payload, payload_ln);
	}

	if ( payload != NULL )
		free(payload);

	return 0;
}

void run()
{

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

	list_process_memory_f = false;
	list_process_modules_f = false;
	list_process_threads_f = false;
	list_process_heaps_f = 0;

	clean_printing = 0;

	print_col_mask = 0;
	print_offset_mask = 4;
	print_hex_mask = 2;
	print_ascii_mask = 1;

	type = TYPE_FILE;
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
		   " * -ix Insert hex byte sequence (destructive!). Where x is an format option.\n"
		   " * -ox Overwrite hex byte sequence (destructive!). Where x is an format option.\n"
		   " * -fx Find hex byte sequence. Where x is an format option.\n"
		   " * * Format options: %c: plain bytes, %c: ascii text, %c: byte, %c: word, %c: double word, %c: quad word.\n"
		   "     Expect for the ascii string, all values have to be passed as hex values.\n"
//		   " * -e:uint8_t Endianess of payload (little: 1, big:2). Defaults to 1 = little endian.\n"
		   " * -d Delete -l bytes from offset -s.\n"
		   " * -t Type of source ['file', 'pid']. Defaults to 'file'. If 'pid', a process id is passed as 'filename'.\n"
		   " * -pid only.\n"
		   " * * -lpx List whole process memory layout.\n"
		   " * * -lpm List all process modules.\n"
		   " * * -lpt List all process threads.\n"
		   " * * -lph List all process heaps.\n"
		   " * * -lphb List all process heaps and its blocks.\n"
		   " * -b Force breaking, not continuous mode.\n"
		   " * -p Plain, not styled text output.\n"
		   " * -h Print this.\n",
		   FORMAT_PLAIN_HEX, FORMAT_ASCII, FORMAT_BYTE, FORMAT_WORD, FORMAT_D_WORD, FORMAT_Q_WORD
	);
	printf("\n");
	printf("Example: ./%s path/to/a.file -s 100 -l 128 -x\n", BINARYNAME);
	printf("Example: ./%s path/to/a.file -ih dead -s 0x100\n", BINARYNAME);
	printf("Example: ./%s path/to/a.file -oh 0bea -s 0x100\n", BINARYNAME);
	printf("Example: ./%s path/to/a.file -fh f001 -s 0x100\n", BINARYNAME);
	printf("Example: ./%s path/to/a.file -d -s 0x100 -l 0x8\n", BINARYNAME);
	printf("\n");
	printf("In continuous mode press ENTER to continue, 'n' to find next or 'q' to quit.\n");
}

void parseArgs(int argc, char** argv)
{
	int start_i = 1;
	int end_i = argc - 1;
	int i, s;
	uint8_t length_found = 0;
	const char* source = NULL;

	if ( isArgOfType(argv[1], "-h") )
	{
		printHelp();
		exit(0);
	}

	if ( argv[1][0] != '-' )
	{
		source = argv[1];
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
		else if ( isArgOfType(argv[i], "-lpx") )
		{
			list_process_memory_f = true;
		}
		else if ( isArgOfType(argv[i], "-lpm") )
		{
			list_process_modules_f = true;
		}
		else if ( isArgOfType(argv[i], "-lpt") )
		{
			list_process_threads_f = true;
		}
		else if ( isArgOfType(argv[i], "-lph") )
		{
			list_process_heaps_f = 1;
		}
		else if ( isArgOfType(argv[i], "-lphb") )
		{
			list_process_heaps_f = 2;
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
#ifdef _WIN32
				printf("length: %llx\n", length);
#else
				printf("length: %lx\n", length);
#endif
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
		else if ( isArgOfType(argv[i], "-t") )
		{
			if ( hasValue("-t", i, end_i) )
			{
				type = parseType(argv[i+1]);
				i++;
			}
		}
		else
		{
			printf("INFO: Unknown arg type \"%s\"\n", argv[i]);
		}
	}

	if ( (find_f + overwrite_f + insert_f + delete_f) > 1 )
	{
		printf("ERROR: overwrite, insert, delete and find have to be used exclusively!\n");
		exit(0);
	}

	if ( delete_f && !length_found )
	{
		printf("ERROR: could not parse length of part to delete!\n");
		exit(0);
	}

	if ( type == TYPE_PID && (insert_f + delete_f) > 0 )
	{
		printf("ERROR: Inserting or deleting is not supported in process mode!\n");
		exit(0);
	}

	if ( start_i == 1 )
		source = argv[i];

	if ( type == TYPE_FILE)
		expandFilePath(source, file_path);
	else
		snprintf(file_path, PATH_MAX, "%s", source);
}

uint8_t isArgOfType(char* arg, char* type)
{
	uint8_t type_ln = strnlen(type, 10);
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

void sanitizeParams(uint32_t pid)
{
	uint8_t col_size;

	if ( print_col_mask == 0 )
		print_col_mask = (print_offset_mask | print_ascii_mask | print_hex_mask);

	if ( insert_f || overwrite_f || delete_f )
		continuous_f = 0;

	col_size = getColSize();

	// normalize length to block size for continuous printing
	if ( continuous_f )
	{
		if ( length % col_size != 0 )
		{
			length = length + col_size - (length % col_size);
		}
	}

	uint8_t info_line_break = 0;
	// check start offset
	if ( type == TYPE_FILE )
		info_line_break = keepStartInFile();
	else if ( type == TYPE_PID )
	{
//		info_line_break = makeStartAndLengthHitAModule(pid, &start);
		info_line_break = makeStartAndLengthHitAccessableMemory(pid, &start);
	}

	// normalize start offset to block size
	if ( !find_f )
	{
		start = normalizeOffset(start, &skip_bytes);
		if ( !continuous_f )
			length += skip_bytes;
	}

	// check length
	if ( type == TYPE_FILE )
		info_line_break = keepLengthInFile();
//	else if ( type == TYPE_PID )
//		info_line_break = keepLengthInModule(pid);

	if ( length == 0 )
	{
		fprintf(stdout, "Info: Length is 0. Setting to %u!\n", DEFAULT_LENGTH);
		length = DEFAULT_LENGTH;
		info_line_break = 1;
	}

	if ( info_line_break )
		printf("\n");
}

uint8_t keepStartInFile()
{
	if ( start > file_size )
	{
#if defined(_WIN32)
		fprintf(stderr, "Info: Start offset %llx is greater the the file_size %llx (%llu)!\nSetting to 0!", start, file_size, file_size);
#else
		fprintf(stderr, "Info: Start offset %lx is greater the the file_size %lx (%lu)!\nSetting to 0!", start, file_size, file_size);
#endif
		start = 0;
		return 1;
	}
	return 0;
}

uint8_t keepLengthInFile()
{
	if ( start + length > file_size )
	{
#if defined(_WIN32)
		printf("Info: Start offset %llu plus length %llu is greater then the file size %llu\nPrinting only to file size.\n",
#else
		printf("Info: Start offset %lu plus length %lu is greater then the file size %lu\nPrinting only to file size.\n",
#endif
			   start + skip_bytes, (continuous_f) ? length : length - skip_bytes, file_size);
		length = file_size - start;
		continuous_f = 0;
		return 1;
	}
	return 0;
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

uint8_t parseType(const char* arg)
{
	if ( strncmp(arg, "file", 10) == 0 )
		return TYPE_FILE;
	else if ( strncmp(arg, "pid", 10) == 0 )
		return TYPE_PID;
	else
	{
		printf("INFO: Could not parse type. Setting it to 'file'!\n");
		return TYPE_FILE;
	}
}
