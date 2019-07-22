#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>

#include "ProcessHandlerLinux.h"
#include "utils/Converter.h"
#include "Globals.h"
#include "Printer.h"
#include "Finder.h"
#include "utils/Helper.h"

int openProcessMemory(uint32_t pid, int* fd);

int openProcessMemory(uint32_t pid, int* fd)
{
	char file[64];
	sprintf(file, "/proc/%lu/mem", pid);
	*fd = open(file, O_RDWR);

	if ( *fd < 0)
		return false;

	return true;
}

uint64_t getSizeOfProcess(uint32_t pid)
{
	int fd = 0;

	if ( !openProcessMemory(pid, fd) )
		return 0;

	close(fd);
	return 0;
}

bool listProcessModules(uint32_t pid)
{
	return true;
}

bool listProcessThreads(uint64_t pid)
{
	return true;
}

bool listProcessMemory(uint32_t pid)
{
	return true;
}


bool listProcessHeaps(uint32_t pid, int type)
{
	return true;
}

uint8_t makeStartAndLengthHitAModule(uint32_t pid, uint64_t* start)
{
	return 0;
}

uint8_t makeStartAndLengthHitAccessableMemory(uint32_t pid, uint64_t* start)
{
	return 0;
}

int writeProcessMemory(uint32_t pid, unsigned char* _payload, uint32_t _payload_ln, uint64_t start)
{
	int fd;

	if ( !openProcessMemory(pid, fd) )
		return 0;

	ptrace(PTRACE_ATTACH, pid, 0, 0);
	waitpid(pid, NULL, 0);

	off_t addr = 0; // target process address
	pwrite(fd, &_payload, _payload_ln, addr);

	ptrace(PTRACE_DETACH, pid, 0, 0);

	close(fd);
	return 0;
}

bool printProcessRegions(uint32_t pid, uint64_t start, uint8_t skip_bytes, unsigned char* needle, uint32_t needle_ln)
{
	int fd;
	char buffer[1024];

	if ( !openProcessMemory(pid, fd) )
		return 0;

	ptrace(PTRACE_ATTACH, pid, 0, 0);
	waitpid(pid, NULL, 0);

	off_t addr = 0; // target process address
	pread(fd, &buffer, sizeof(buffer), addr);

	ptrace(PTRACE_DETACH, pid, 0, 0);

	close(fd);

	return true;
}

