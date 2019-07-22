#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ptrace.h>

#include "ProcessHandlerLinux.h"
#include "utils/Converter.h"
#include "Globals.h"
#include "Printer.h"
#include "Finder.h"
#include "utils/Helper.h"

bool openProcessFile(uint32_t pid, int *fd, char *type, int flag);
bool fopenProcessFile(uint32_t pid, FILE **fp, char *type, const char* mode);
bool openProcessMemory(uint32_t pid, int *fd, int flag);
bool openProcessMaps(uint32_t pid, int *fd);
bool fopenProcessMaps(uint32_t pid, FILE **fp);

size_t split(char* str, char* delimiter, char** bucket, const size_t bucket_max);

/**
 * Open proc pid file
 *
 * @param	pid uint32_t the process id
 * @param	fd int* the file descriptor
 * @param	tpye char* the proc file to open
 * @param	flag int the access flag: O_RDONLY, O_RDWR, O_WRONLY
 */
bool openProcessFile(uint32_t pid, int *fd, char *type, int flag)
{
	char file[64];
	sprintf(file, "/proc/%u/%s", pid, type);
	*fd = open(file, flag);

	if ( *fd < 0 )
		return false;

	return true;
}

/**
 * Open proc pid file
 *
 * @param	pid uint32_t the process id
 * @param	fp FILE** the file descriptor
 * @param	tpye char* the proc file to open
 * @param	flag int the access flag: O_RDONLY, O_RDWR, O_WRONLY
 */
bool fopenProcessFile(uint32_t pid, FILE **fp, char *type, const char* mode)
{
	char file[64];
	sprintf(file, "/proc/%u/%s", pid, type);
	*fp = fopen(file, mode);

	if ( !(*fp) )
		return false;

	return true;
}

bool openProcessMemory(uint32_t pid, int *fd, int flag)
{
	return openProcessFile(pid, fd, "mem", flag);
}

bool openProcessMaps(uint32_t pid, int *fd)
{
	return openProcessFile(pid, fd, "maps", O_RDONLY);
}

bool fopenProcessMaps(uint32_t pid, FILE **fp)
{
	return fopenProcessFile(pid, fp, "maps", "r");
}

uint64_t getSizeOfProcess(uint32_t pid)
{
	int fd = 0;

	if ( !openProcessMaps(pid, &fd) )
		return 0;

	close(fd);
	return 1024;
}

bool listProcessModules(uint32_t pid)
{
	return true;
}

bool listProcessThreads(uint64_t pid)
{
	return true;
}

size_t split(char* str, char* delimiter, char** bucket, const size_t bucket_max)
{
	char *pch;
	pch = strtok(str, delimiter);
	size_t token_id = 0;

	while ( pch != NULL )
	{
//		printf("%s\n", pch);
		bucket[token_id] = pch;
		pch = strtok(NULL, delimiter);

		token_id++;
		if ( token_id >= bucket_max )
			break;
	}
	return token_id;
}

/**
 * Read out process maps: /proc/pid/maps
 * Format: address perms offset dev inode pathname
 *
 * perms:
 * 	r = read
 * 	w = write
 * 	x = execute
 * 	s = shared
 * 	p = private (copy on write)
 */
bool listProcessMemory(uint32_t pid)
{
//	int fd;
	FILE* fp = NULL;
	const uint16_t line_size = 512;
	char line[513];
	const uint16_t bucket_max = 8;
	uint16_t bucket_ln = 0;
	char* bucket[8];
	const uint8_t from_to_max = 2;
	uint8_t from_to_ln = 0;
	char* from_to[2];
	size_t n;
	uint16_t i=0;
	int s;
	uint64_t address, size;
	uint8_t widths[6] = { 16, 5, 8, 5, 8, 8 };

//	if ( !openProcessMaps(pid, &fd) )
//		return 0;
	if ( !fopenProcessMaps(pid, &fp) )
		return 0;


//	while( (n = read(fd, &line, line_size)) )
//	{
//		line[line_size] = 0;
//		printf("%d: %s", ++i, line);
//	}

	printf("%-*s | %-*s | %-*s | %*s | %*s | %*s | %*s\n",
			widths[0]+2, "address", widths[0]+2, "size", widths[1], "perms", widths[2], "offset", widths[3], "dev", widths[4], "inode", widths[5], "pathname");
	printf("------------------------------------------------------------------------------------------\n");
	while ( fgets(line, line_size, fp) )
	{
		line[line_size] = 0;
//		printf("%s (%u)\n", line, found_tokens);
		bucket_ln = split(line, " \n", bucket, bucket_max);

		if ( bucket_ln < 1 )
			break;

		from_to_ln = split(bucket[0], "-", from_to, from_to_max);
		if ( from_to_ln != 2 )
			break;

		s = parseUint64(from_to[0], &address, 16);
		if ( s != 0 )
			break;
		printf("0x%0*lx | ", widths[0], address);

		s = parseUint64(from_to[1], &size, 16);
		if ( s != 0 )
			break;
		size = size - address;
		printf("0x%0*lx | ", widths[0], size);


		for ( i = 1; i < bucket_ln; i++ )
		{
			printf("%*s | ", widths[i], bucket[i]);
		}
		printf("\n");
	}

//	close(fd);
	fclose(fp);
	return true;
}

bool listProcessHeaps(uint32_t pid, int type)
{
	return true;
}

uint8_t makeStartAndLengthHitAModule(uint32_t pid, uint64_t *start)
{
	return 0;
}

uint8_t makeStartAndLengthHitAccessableMemory(uint32_t pid, uint64_t *start)
{
	return 0;
}

int writeProcessMemory(uint32_t pid, unsigned char *_payload,
		uint32_t _payload_ln, uint64_t start)
{
	int fd;

	if ( !openProcessMemory(pid, &fd, O_RDWR) )
		return 0;

	ptrace(PTRACE_ATTACH, pid, 0, 0);
	waitpid(pid, NULL, 0);

	off_t addr = 0; // target process address
	pwrite(fd, &_payload, _payload_ln, addr);

	ptrace(PTRACE_DETACH, pid, 0, 0);

	close(fd);
	return 0;
}

bool printProcessRegions(uint32_t pid, uint64_t start, uint8_t skip_bytes,
		unsigned char *needle, uint32_t needle_ln)
{
	int fd;
	char buffer[1024];

	if ( !openProcessMemory(pid, &fd, O_RDONLY) )
		return 0;

	ptrace(PTRACE_ATTACH, pid, 0, 0);
	waitpid(pid, NULL, 0);

	off_t addr = 0; // target process address
	pread(fd, &buffer, sizeof(buffer), addr);

	ptrace(PTRACE_DETACH, pid, 0, 0);

	close(fd);

	return true;
}

