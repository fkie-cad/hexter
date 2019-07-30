#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>

#include <windows.h>
#include <tlhelp32.h>
#include <conio.h>
#include <intsafe.h>
#include <psapi.h>

#include "ProcessHandlerWin.h"
#include "utils/Converter.h"
#include "Globals.h"
#include "Printer.h"
#include "Finder.h"
#include "utils/Helper.h"

typedef int (*MemInfoCallback)(HANDLE, MEMORY_BASIC_INFORMATION*);

//int printModuleProcessMemory(HANDLE process, MODULEENTRY32* me32, uint64_t base_off, uint64_t found);
size_t readProcessBlock(BYTE* base_addr, DWORD base_size, uint64_t base_off, HANDLE process, unsigned char* block);
BOOL getNextPrintableRegion(HANDLE process, MEMORY_BASIC_INFORMATION* info, unsigned char** p, char* file_name,
							int print_s, PVOID last_base);
BOOL queryNextRegion(HANDLE process, unsigned char** p, MEMORY_BASIC_INFORMATION* info);
BOOL queryNextAccessibleRegion(HANDLE process, unsigned char** p, MEMORY_BASIC_INFORMATION* info);
BOOL queryNextAccessibleBaseRegion(HANDLE process, unsigned char** p, MEMORY_BASIC_INFORMATION* info);
size_t printMemoryBlock(HANDLE process, BYTE* base_addr, uint64_t base_off, DWORD base_size, unsigned char* buffer);
void printError(LPTSTR lpszFunction, DWORD last_error);
BOOL getModule(uint64_t address, HANDLE snap, MODULEENTRY32* me32);
uint64_t findNeedleInProcessMemoryBlock(BYTE* base_addr, DWORD base_size, uint64_t offset, HANDLE process, const unsigned char* needle, uint32_t needle_ln);
BOOL openSnapAndME(HANDLE* snap, MODULEENTRY32* me32, uint32_t pid, DWORD dwFlags);
BOOL openSnap(HANDLE* snap, uint32_t pid, DWORD dwFlags);
BOOL openME(HANDLE* snap, MODULEENTRY32* me32);
BOOL openProcessAndSnapAndME(HANDLE* process, HANDLE* snap, MODULEENTRY32* me32, uint32_t pid, DWORD dwFlags);
void initME32(MODULEENTRY32* me32);
BOOL addressIsInRegionRange(uint64_t address, uint64_t base, DWORD size);
void ProcessHandler_cleanUp(HANDLE snap, HANDLE process);
BOOL openProcess(HANDLE* process, uint32_t pid);
bool getProcessHeapListSnapshot(HANDLE* hHeapSnap, HEAPLIST32* hl, DWORD pid);
void listProcessHeapBlocks(uint32_t pid, ULONG_PTR base);
char* getHEFlagString(DWORD flag);
char* getHLFlagString(DWORD flag);
char* getProtectString(DWORD protect);
int printMemoryInfo(HANDLE process, MEMORY_BASIC_INFORMATION* info);
int iterateProcessMemory(HANDLE process, MEMORY_BASIC_INFORMATION* info, MemInfoCallback cb);
char* getMemoryStateString(DWORD state);
char* getMemoryTypeString(DWORD type);
int printRegionProcessMemory(HANDLE process, BYTE* base_addr, uint64_t base_off, SIZE_T size, uint64_t found, char* reg_name);
BOOL getRegion(uint64_t address, HANDLE process, MEMORY_BASIC_INFORMATION* info);
BOOL getRegionName(HANDLE process, PVOID base, char* file_name);
BOOL notAccessibleRegion(MEMORY_BASIC_INFORMATION* info);
BOOL isAccessibleRegion(MEMORY_BASIC_INFORMATION* info);
BOOL setUnflagedRegionProtection(HANDLE process, MEMORY_BASIC_INFORMATION* info, DWORD new_protect, DWORD* old_protect);
BOOL keepLengthInModule(MEMORY_BASIC_INFORMATION* info, uint64_t start, uint64_t *length);
void printRegionInfo(MEMORY_BASIC_INFORMATION* info, const char* file_name);

static unsigned char* p_needle = NULL;
static uint32_t p_needle_ln;

static HANDLE hStdout;
static WORD wOldColorAttrs;
static HANDLE hStdout;
static CONSOLE_SCREEN_BUFFER_INFO csbiInfo;

uint64_t getSizeOfProcess(uint32_t pid)
{
	size_t p_size = 0;
	HANDLE snap = INVALID_HANDLE_VALUE;
	MODULEENTRY32 me32;

//	if ( !openProcess(&process, pid) )
//		return 0;

	if ( !openSnapAndME(&snap, &me32, pid, TH32CS_SNAPMODULE))
		return 0;

	p_size = me32.modBaseSize;

	CloseHandle(snap);

	return p_size;
}

/**
 * Try to hit accessible memory with start offset.
 * If not hit, set start to first accessible region.
 * Check if length fits in region, otherwise crop it to fit.
 *
 * @param pid uint32_t
 * @param start uint64_t*
 * @return
 */
uint8_t makeStartAndLengthHitAccessableMemory(uint32_t pid, uint64_t* start)
{
	unsigned char *p = NULL;
//	unsigned char *first_region = NULL;
	MEMORY_BASIC_INFORMATION info;
	HANDLE process = NULL;
	PVOID base_addr;

	uint8_t info_line_break = 0;

	if ( !openProcess(&process, pid) )
		return false;

	for ( p = NULL;
		  VirtualQueryEx(process, p, &info, sizeof(info)) == sizeof(info);
		  p += info.RegionSize )
	{
		if ( info.Protect == PAGE_NOACCESS || info.Protect == 0 )
			continue;
//		if ( first_region == NULL )
//			first_region = p;

		base_addr = info.BaseAddress;
//		base_addr = info.AllocationBase;
		if ( *start < (uint64_t) base_addr )
			break;

		if ( addressIsInRegionRange(*start, (uint64_t) base_addr, info.RegionSize) )
		{
			info_line_break = keepLengthInModule(&info, *start, &length);
			CloseHandle(process);
			return info_line_break;
		}
	}

//	if ( first_region == NULL )
	if ( p == NULL )
	{
		CloseHandle(process);
		return 2;
	}

//	if ( VirtualQueryEx(process, first_region, &info, sizeof(info)) != sizeof(info) )
	if ( VirtualQueryEx(process, p, &info, sizeof(info)) != sizeof(info) )
	{
		CloseHandle(process);
		return 3;
	}

	if ( (*start) > 0 )
	{
		printf("Info: Start offset %llx does not hit a module!\nSetting it to %p!", (*start), info.AllocationBase);
		info_line_break = 1;
	}
	(*start) = (uint64_t) info.AllocationBase;
	if ( keepLengthInModule(&info, *start, &length) )
		info_line_break = 1;

	return info_line_break;
}

BOOL keepLengthInModule(MEMORY_BASIC_INFORMATION* info, uint64_t start, uint64_t *length)
{
	printf("TODO: get size of whole module, the region belongs to.\n");
	DWORD base_off = start - (uint64_t) info->BaseAddress;
	if ( base_off + *length > info->RegionSize )
	{
		printf("Info: Length %llx does not fit in region!\nSetting it to %lx!\n", *length, info->RegionSize - base_off);
		*length = info->RegionSize - base_off;
		return 1;
	}
	return 0;
}

BOOL listProcessModules(uint32_t pid)
{
	uint16_t i = 0;
	HANDLE snap = INVALID_HANDLE_VALUE;
	MODULEENTRY32 me32;

	if ( !openSnapAndME(&snap, &me32, pid, TH32CS_SNAPMODULE))
		return FALSE;

	printf("List of modules:\n");
	printf("[Process ID | g count | p count | Base address | Base size]\n");
	do
	{
		i++;

		printf("%2u. Module: %s\n", i, me32.szModule);
		printf("    - Path = %s\n", me32.szExePath);
		printf("    - 0x%08lX |", me32.th32ProcessID);
		printf(" 0x%04lX |", me32.GlblcntUsage);
		printf(" 0x%04lX |", me32.ProccntUsage);
		printf(" 0x%p |", me32.modBaseAddr);
		printf(" %lu\n", me32.modBaseSize);
	}
	while ( Module32Next(snap, &me32) );
	printf("\n");

	CloseHandle(snap);

	return TRUE;
}

BOOL listProcessThreads(uint64_t pid)
{
	HANDLE snap = INVALID_HANDLE_VALUE;
	THREADENTRY32 te32;
	uint16_t i = 0;

	if ( !openSnap(&snap, pid, TH32CS_SNAPTHREAD) )
		return FALSE;

	memset(&te32, 0, sizeof(THREADENTRY32));
	te32.dwSize = sizeof(THREADENTRY32);

	if ( !Thread32First(snap, &te32) )
	{
		printf("Error: Thread32First\n");
		CloseHandle(snap);
		return FALSE;
	}

	printf("List of threads:\n");
	printf("nr | ThreadID   | OwnerProcessID | BasePri\n");
	printf("------------------------------------------\n");
	do
	{
		if ( te32.th32OwnerProcessID == pid )
		{
			i++;
			printf("%2u | 0x%-8lx | %14lu | %7lu\n",
					i, te32.th32ThreadID, te32.th32OwnerProcessID, te32.tpBasePri);
		}
	}
	while ( Thread32Next(snap, &te32) );
	printf("\n");

	CloseHandle(snap);
	return TRUE;
}

int writeProcessMemory(uint32_t pid, unsigned char* payload, uint32_t payload_ln, uint64_t start)
{
	HANDLE process;
	MEMORY_BASIC_INFORMATION info;

	PVOID base_addr = (PVOID) start;
	BOOL s;
	uint8_t i;
	SIZE_T bytes_written = 0;
	DWORD last_error;
	DWORD old_protect;

	const DWORD nSize = 512;
	char f_path[512];
	char* file_name;
	memset(f_path, 0, 512);

	if ( !openProcess(&process, pid) )
		return 1;

	s = getRegion(start, process, &info);
	if ( !s )
	{
		printf("Error: No region hit for writing\n");
		CloseHandle(process);
		return 2;
	}

	GetModuleFileNameExA(process, info.AllocationBase, f_path, nSize);
	getFileNameL(f_path, &file_name);

//	printf("Write into %s:\n", file_name);
//	printf("payload_ln %u\n", payload_ln);
//	for ( i = 0; i < payload_ln; i++ )
//		printf("%02x|", payload[i]);
//	printf("\n");

	s = VirtualProtectEx(process, base_addr, payload_ln, PAGE_EXECUTE_READWRITE, &old_protect);
	if ( !s )
	{
		last_error = GetLastError();
		printf(" - Error (%lu): VirtualProtect at 0x%llx\n", last_error, (uint64_t) base_addr);
		printError("VirtualProtect", last_error);
	}

	s = WriteProcessMemory(process, base_addr, payload, payload_ln, &bytes_written);

	if ( !s )
	{
		last_error = GetLastError();
		printf(" - Error (%lu): WriteProcessMemory %lu bytes at 0x%p\n", last_error, bytes_written, base_addr);
		printError("WriteProcessMemory", last_error);
	}

	s = VirtualProtectEx(process, base_addr, payload_ln, old_protect, &old_protect);
	if ( !s )
	{
		last_error = GetLastError();
		printf(" - Error (%lu): VirtualProtect at 0x%llx\n", last_error, (uint64_t) base_addr);
		printError("VirtualProtect", last_error);
	}

	CloseHandle(process);

	return 0;
}

/**
 *
 * @param pid
 * @param start uint64_t absolute start offset
 * @param skip_bytes
 * @param needle
 * @param needle_ln
 * @return
 */
BOOL printProcessRegions(uint32_t pid, uint64_t start, uint8_t skip_bytes, unsigned char* needle, uint32_t needle_ln)
{
	HANDLE process;
	MEMORY_BASIC_INFORMATION info;
	BOOL s;
	int print_s = 0;
	unsigned char *p = NULL;
	PVOID last_base = 0;
	DWORD old_protect = 0;

	uint64_t base_off;
	uint64_t found = FIND_FAILURE;

	char file_name[PATH_MAX];

	p_needle = needle;
	p_needle_ln = needle_ln;

	if ( !openProcess(&process, pid) )
		return FALSE;

	s = getRegion(start, process, &info);
	if ( !s )
	{
		printf("ERROR: No region hit!\n");
		CloseHandle(process);
		return FALSE;
	}

	if ( find_f )
		Finder_initFailure(p_needle, p_needle_ln);

	getRegionName(process, info.AllocationBase, file_name);
	printf(" - file_name: %s\n", file_name);
	p = info.BaseAddress;
	last_base = info.AllocationBase;
	base_off = start - (uint64_t) info.AllocationBase; // ?? not address ??
	printRegionInfo(&info, file_name);

	while ( s )
	{
		setUnflagedRegionProtection(process, &info, PAGE_READONLY, &old_protect);

		if ( find_f )
		{
			found = findNeedleInProcessMemoryBlock(info.BaseAddress, info.RegionSize, base_off, process, p_needle,
												   p_needle_ln);
			if ( found == FIND_FAILURE )
			{
				if ( !getNextPrintableRegion(process, &info, &p, file_name, print_s, last_base) )
					break;

				last_base = info.AllocationBase;
				continue;
			}
			else
			{
				found = found - (uint64_t) info.BaseAddress;
				base_off = normalizeOffset(found, &skip_bytes);
				Printer_setHiglightBytes(p_needle_ln);
				Printer_setHiglightWait(skip_bytes);
				skip_bytes = 0;
			}
		}

//		Printer_setSkipBytes(skip_bytes);
		print_s = printRegionProcessMemory(process, info.BaseAddress, base_off, info.RegionSize, found, file_name);

		setUnflagedRegionProtection(process, &info, old_protect, &old_protect);

		if ( !getNextPrintableRegion(process, &info, &p, file_name, print_s, last_base) )
			break;

		if ( print_s == 1 )
			printRegionInfo(&info, file_name);
		last_base = info.AllocationBase;
		base_off = 0;
	}

	CloseHandle(process);
	Finder_cleanUp();

	return TRUE;
}

void printRegionInfo(MEMORY_BASIC_INFORMATION* info, const char* file_name)
{
	printf("%s (0x%p - 0x%p):\n", file_name, (BYTE*) info->AllocationBase, (BYTE*) info->AllocationBase + info->RegionSize);
}

BOOL setUnflagedRegionProtection(HANDLE process, MEMORY_BASIC_INFORMATION* info, DWORD new_protect, DWORD* old_protect)
{
	BOOL s;
	if ( strncmp(getProtectString(info->Protect), "None", 6) == 0 )
	{
		s = VirtualProtectEx(process, info->BaseAddress, info->RegionSize, new_protect, old_protect);
		if ( !s )
		{
			printf(" - Error (%lu): VirtualProtect at 0x%llx\n", GetLastError(), (uint64_t) info->BaseAddress);
			printError("VirtualProtect", GetLastError());
			return FALSE;
		}
	}
	return TRUE;
}

BOOL getNextPrintableRegion(HANDLE process, MEMORY_BASIC_INFORMATION* info, unsigned char** p, char* file_name,
							int print_s, PVOID last_base)
{
	BOOL s;

	if ( print_s == 0 )
		s = queryNextAccessibleRegion(process, p, info);
	else
		s = queryNextAccessibleBaseRegion(process, p, info);

	if ( !s )
		return FALSE;

//	printf("last_base: %p\n", last_base);
//	printf("info.AllocationBase: %p\n", info->AllocationBase);

	getRegionName(process, info->AllocationBase, file_name);
//	printf("file_name: %s\n", *file_name);
	if ( last_base != info->AllocationBase )
	{
		if ( !confirmContinueWithNextRegion(file_name, info->AllocationBase) )
			return FALSE;
	}
	return TRUE;
}

BOOL queryNextAccessibleRegion(HANDLE process, unsigned char** p, MEMORY_BASIC_INFORMATION* info)
{
	BOOL s;
	s = queryNextRegion(process, p, info);

	while ( s )
	{
		if ( !isAccessibleRegion(info) )
			s = queryNextRegion(process, p, info);
		else
			break;
	}
	return s;
}

BOOL queryNextAccessibleBaseRegion(HANDLE process, unsigned char** p, MEMORY_BASIC_INFORMATION* info)
{
	BOOL s;
	PVOID old_base;

	old_base = info->AllocationBase;
	s = queryNextRegion(process, p, info);

	while ( s )
	{
		if ( old_base == info->AllocationBase || !isAccessibleRegion(info) )
			s = queryNextRegion(process, p, info);
		else
			break;
	}
	return s;
}

BOOL queryNextRegion(HANDLE process, unsigned char** p, MEMORY_BASIC_INFORMATION* info)
{
	SIZE_T query_size = 0;

	*p += info->RegionSize;
	query_size = VirtualQueryEx(process, *p, info, sizeof(*info));
//	printf("query_size: %lu\n", query_size);
//	printf("sizeof(*info): %lu\n", sizeof(*info));

	if ( query_size != sizeof(*info) )
	{
		printf("ERROR (0x%08lx) VirtualQueryEx: could not query info!\n", GetLastError());
		printError("queryNextRegion", GetLastError());
		return FALSE;
	}

	return TRUE;
}

BOOL getRegionName(HANDLE process, PVOID base, char* file_name)
{
	const DWORD f_path_size = PATH_MAX;
	char f_path[PATH_MAX];
	DWORD s;
	printf("getRegionName()\n");

//	GetModuleBaseNameA(process, base, f_path, f_path_size);
	s = GetMappedFileNameA(process, base, f_path, f_path_size);
	printf(" - s: %lu\n", s);
	if ( s == 0 )
	{
		*file_name = NULL;
		return FALSE;
	}
	printf(" - f_path: %s\n", f_path);
	f_path[f_path_size-1] = 0;
	printf(" - f_path: %s\n", f_path);
	getFileName(f_path, file_name);
	printf(" - file_name: %s\n", file_name);

	return TRUE;
}

/**
 *
 * @param process
 * @param me32
 * @param base_off uint32_t base offset in module to start at printing
 * @return 0 if end of block is reached, 1 if forced to quit
 */
int printRegionProcessMemory(HANDLE process, BYTE* base_addr, uint64_t base_off, SIZE_T size, uint64_t found,
							 char* reg_name)
{
	size_t n_size = length;
	unsigned char buffer[BLOCKSIZE_LARGE] = {0};
	char input;
	uint8_t skip_bytes;
	int s = 0;

	n_size = printMemoryBlock(process, base_addr, base_off, size, buffer);
	base_off += n_size;

	if ( !continuous_f )
		return 1;

	while ( n_size && n_size == length )
	{
		input = _getch();

		if ( input == ENTER )
			n_size = printMemoryBlock(process, base_addr, base_off, size, buffer);
		else if ( find_f && input == 'n' )
		{
			found = findNeedleInProcessMemoryBlock(base_addr, size, found + p_needle_ln, process, p_needle, p_needle_ln);
			if ( found == FIND_FAILURE )
			{
				s = 0;
				break;
			}
			found -= (uint64_t) base_addr;
			base_off = normalizeOffset(found, &skip_bytes);
			Printer_setHiglightBytes(p_needle_ln);
			Printer_setHiglightWait(skip_bytes);
			skip_bytes = 0;

			printf("\n");
			n_size = printMemoryBlock(process, base_addr, base_off, size, buffer);
		}
		else if ( input == 'q' )
		{
			s = 1;
			break;
		}

		base_off += n_size;
	}

	return s;
}

/**
 *
 * @param me32
 * @param process
 * @param base_off
 * @param base_addr
 * @param buffer
 * @return
 */
size_t
printMemoryBlock(HANDLE process, BYTE* base_addr, uint64_t base_off, DWORD base_size, unsigned char* buffer)
{
	size_t n_size = readProcessBlock(base_addr, base_size, base_off, process, buffer);
	uint8_t offset_width = countHexWidth64((uint64_t)base_addr+base_size);
	if ( n_size )
		printLine(buffer, (uint64_t)base_addr + base_off, n_size, offset_width);

	return n_size;
}

/**
 *
 * @param me32 MODULEENTRY32*
 * @param process HANDLE
 * @param base_off uint64_t
 * @param block char*
 * @return
 */
size_t readProcessBlock(BYTE* base_addr, DWORD base_size, uint64_t base_off, HANDLE process, unsigned char* block)
{
	SIZE_T bytes_read = 0;
	size_t n_size = length;
	BOOL s;

	if ( base_off + n_size > base_size )
	{
		n_size = base_size - base_off;
		if ( n_size == 0 )
			return 0;
	}

	s = ReadProcessMemory(process, (LPCVOID) (base_addr + base_off), block, n_size, &bytes_read);

	if ( !s )
	{
		printf(" - Error (%lu): ReadProcessMemory %lu bytes at 0x%llx\n", GetLastError(), bytes_read, (uint64_t)base_addr + base_off);
		return 0;
	}

	return n_size;
}

/**
 * Get accessible region hit by address.
 *
 * @param address
 * @param process HANDLE
 * @param info MEMORY_BASIC_INFORMATION*
 * @return BOOL
 */
BOOL getRegion(uint64_t address, HANDLE process, MEMORY_BASIC_INFORMATION* info)
{
	unsigned char *p = NULL;

	for ( p = NULL;
		  VirtualQueryEx(process, p, info, sizeof(*info)) == sizeof(*info);
		  p += info->RegionSize )
	{
		if ( info->Protect == PAGE_NOACCESS )
//		if ( !isAccessibleRegion(info) )
			continue;

		if ( addressIsInRegionRange(address, (uint64_t) info->AllocationBase, info->RegionSize) )
			return TRUE;
	}
	return FALSE;
}

/**
 * Get module hit by address.
 *
 * @param address
 * @param snap
 * @param me32
 * @return
 */
BOOL getModule(uint64_t address, HANDLE snap, MODULEENTRY32* me32)
{
	do
	{
		if ( addressIsInRegionRange(address, (uint64_t) me32->modBaseAddr, me32->modBaseSize))
			return TRUE;
	}
	while ( Module32Next(snap, me32) );
	return FALSE;
}

BOOL addressIsInRegionRange(uint64_t address, uint64_t base, DWORD size)
{
	uint64_t end = base + size;
	return base <= address && address <= end;
}

/**
 * Find needle in a process memory block.
 *
 * @param me32
 * @param process
 * @param needle
 * @param needle_ln
 * @param offset
 * @return uint64_t absolute found address
 */
uint64_t
findNeedleInProcessMemoryBlock(BYTE* base_addr, DWORD base_size, uint64_t offset, HANDLE process, const unsigned char* needle, uint32_t needle_ln)
{
	uint64_t found = FIND_FAILURE;
	uint64_t block_i;
	uint64_t j = 0;
	uint64_t base_off = offset;
	size_t n_size = length;
	unsigned char buf[BLOCKSIZE_LARGE] = {0};

	debug_info("Find: ");
	for ( block_i = 0; block_i < needle_ln; block_i++ )
		debug_info("%02x", p_needle[block_i]);
	debug_info("\n");

	while ( n_size && n_size == length )
	{
		n_size = readProcessBlock(base_addr, base_size, base_off, process, buf);
		block_i = findNeedleInBlock(needle, needle_ln, buf, &j, n_size);

		if ( j == needle_ln )
		{
			found = (uint64_t) base_addr + base_off + block_i - needle_ln + 1;
			break;
		}

		base_off += block_i;
	}

	return found;
}

BOOL openSnapAndME(HANDLE* snap, MODULEENTRY32* me32, uint32_t pid, DWORD dwFlags)
{
	if ( !openSnap(snap, pid, dwFlags))
		return FALSE;

	if ( !openME(snap, me32) )
	{
		CloseHandle((*snap));
		return FALSE;
	}

	return TRUE;
}

BOOL openProcessAndSnapAndME(HANDLE* process, HANDLE* snap, MODULEENTRY32* me32, uint32_t pid, DWORD dwFlags)
{
	if ( !openProcess(process, pid) )
		return FALSE;

	if ( !openSnap(snap, pid, dwFlags))
	{
		CloseHandle((*process));
		return FALSE;
	}

	if ( !openME(snap, me32) )
	{
		ProcessHandler_cleanUp((*snap), (*process));
		return FALSE;
	}

	return TRUE;
}

BOOL openProcess(HANDLE* process, uint32_t pid)
{
//	uint32_t lpExitCode = 0;

	(*process) = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if ((*process) == NULL)
	{
		printf("ERROR (%lu): OpenProcess %u failed\n", GetLastError(), pid);
		return FALSE;
	}
//	GetExitCodeProcess(process, &lpExitCode);
//	if ( lpExitCode != STILL_ACTIVE )
//	{
//		printf("Process %u not running.\n", pid);
//		return FALSE;
//	}
	return TRUE;
}

BOOL openSnap(HANDLE* snap, uint32_t pid, DWORD dwFlags)
{
	(*snap) = CreateToolhelp32Snapshot(dwFlags, pid);
	if ( (*snap) == INVALID_HANDLE_VALUE)
	{
		printf("Error (%lu): CreateToolhelp32Snapshot\n", GetLastError());
		return FALSE;
	}
	return TRUE;
}

BOOL openME(HANDLE* snap, MODULEENTRY32* me32)
{
	DWORD last_error = 0;
	initME32(me32);
	if ( !Module32First((*snap), me32))
	{
		last_error = GetLastError();
		printf("Error (%lu): Module32First", last_error);
		return FALSE;
	}
	return TRUE;
}

void initME32(MODULEENTRY32* me32)
{
	memset(me32, 0, sizeof(MODULEENTRY32));
	me32->dwSize = sizeof(MODULEENTRY32);
}

void ProcessHandler_cleanUp(HANDLE snap, HANDLE process)
{
	if ( snap ) CloseHandle(snap);
	if ( process ) CloseHandle(process);
}

#include <strsafe.h>
#include <minwinbase.h>

void printError(LPTSTR lpszFunction, DWORD last_error)
{
	// Retrieve the system error message for the last-error code

	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;

	FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			last_error,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &lpMsgBuf,
			0, NULL);

	lpDisplayBuf = (LPVOID) LocalAlloc(LMEM_ZEROINIT,
									   (lstrlen((LPCTSTR) lpMsgBuf) + lstrlen((LPCTSTR) lpszFunction) + 40) *
									   sizeof(TCHAR));
	StringCchPrintf((LPTSTR) lpDisplayBuf,
					LocalSize(lpDisplayBuf) / sizeof(TCHAR),
					TEXT("%s failed with error %d: %s"),
					lpszFunction, last_error, lpMsgBuf);
	printf("ERROR: %s\n", (LPCTSTR) lpDisplayBuf);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}

int iterateProcessMemory(HANDLE process, MEMORY_BASIC_INFORMATION* info, MemInfoCallback cb)
{
	unsigned char *p = NULL;
	int s = 0;

	for ( p = NULL;
		  VirtualQueryEx(process, p, info, sizeof(*info)) == sizeof(*info);
		  p += info->RegionSize )
	{
		s = cb(process, info);
		if ( s != 0 )
			return s;
	}

	return 0;
}

bool listProcessMemory(uint32_t pid)
{
	HANDLE process = NULL;
	MEMORY_BASIC_INFORMATION info;

	if ( !openProcess(&process, pid) )
		return false;

	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(hStdout, &csbiInfo);
	wOldColorAttrs = csbiInfo.wAttributes;

	printf("%-18s | %-18s | %10s | %-9s | %-11s | %-18s | %-18s | %s [| %s]\n",
			"BaseAddress", "AllocationBase", "RegionSize", "State", "Type", "Protect", "AllocationProtect", "Name", "info");
	printf("----------------------------------------------------------------------------------------------------------------------------------------\n");
	iterateProcessMemory(process, &info, &printMemoryInfo);
	printf("\n");

	return true;
}

int printMemoryInfo(HANDLE process, MEMORY_BASIC_INFORMATION* info)
{
//	uint64_t usage = 0;
	char f_path[PATH_MAX+1];
	char* file_name;
	DWORD f_path_size = PATH_MAX;
	const char* SEPARATOR = " | ";
	int guard = 0, nocache = 0;

	if ( !isAccessibleRegion(info) )
		SetConsoleTextAttribute(hStdout, FOREGROUND_INTENSITY);

	printf("0x%p | 0x%p | 0x%8lx | ", info->BaseAddress, info->AllocationBase, info->RegionSize);

	printf("%-9s%s", getMemoryStateString(info->State), SEPARATOR);

	printf("%-11s%s" , getMemoryTypeString(info->Type), SEPARATOR);

//	if ((info->State == MEM_COMMIT) && (info->Type == MEM_PRIVATE))
//		usage +=info->RegionSize;

	guard = 0;
	nocache = 0;
	if ( info->AllocationProtect & PAGE_NOCACHE )
		nocache = 1;
	if ( info->AllocationProtect & PAGE_GUARD )
		guard = 1;

	printf("%-18s%s", getProtectString(info->Protect), SEPARATOR);

	info->AllocationProtect &= ~(PAGE_GUARD | PAGE_NOCACHE);
	printf("%-18s%s", getProtectString(info->AllocationProtect), SEPARATOR);

	memset(f_path, 0, MAX_PATH);
//	if ( GetModuleBaseNameA(process, info->AllocationBase, f_path, f_path_size) )
	if ( GetMappedFileNameA(process, info->AllocationBase, f_path, f_path_size) )
	{
		f_path[PATH_MAX] = 0;
		getFileNameL(f_path, &file_name);
		printf("%s", file_name);
	}

	if ( guard )
		printf("%sguard page", SEPARATOR);
	if ( nocache )
		printf("%snon-cachable", SEPARATOR);

	printf("\n");

//	if ( notAccessibleRegion(info) )
	if ( !isAccessibleRegion(info) )
		SetConsoleTextAttribute(hStdout, wOldColorAttrs);

	return 0;
}

BOOL notAccessibleRegion(MEMORY_BASIC_INFORMATION* info)
{
//	info->State == MEM_RESERVE
	return ( info->State == MEM_FREE && info->Protect == PAGE_NOACCESS ) ||
			( info->State == MEM_RESERVE && info->Type == MEM_PRIVATE && info->Protect == 0 );
}

BOOL isAccessibleRegion(MEMORY_BASIC_INFORMATION* info)
{
//	return	!( info->State == MEM_FREE && info->Protect == PAGE_NOACCESS )
	return	info->Protect != PAGE_NOACCESS
			&&
			!( info->State == MEM_RESERVE && info->Type == MEM_PRIVATE && info->Protect == 0 );
}

char* getMemoryStateString(DWORD state)
{
	switch (state) {
		case MEM_COMMIT:
			return("Committed");
		case MEM_RESERVE:
			return("Reserved");
		case MEM_FREE:
			return("Free");
		default:
			return("None");
	}
}

char* getMemoryTypeString(DWORD type)
{
	switch (type) {
		case MEM_IMAGE:
			return("Code Module");
		case MEM_MAPPED:
			return("Mapped");
		case MEM_PRIVATE:
			return("Private");
		default:
			return("None");
	}
}

char* getProtectString(DWORD protect)
{
	switch ( protect )
	{
		case PAGE_NOACCESS:
			return("No Access");
		case PAGE_READONLY:
			return("Read Only");
		case PAGE_READWRITE:
			return("Read/Write");
		case PAGE_WRITECOPY:
			return("Copy on Write");
		case PAGE_EXECUTE:
			return("Execute only");
		case PAGE_EXECUTE_READ:
			return("Execute/Read");
		case PAGE_EXECUTE_READWRITE:
			return("Execute/Read/Write");
		case PAGE_EXECUTE_WRITECOPY:
			return("COW Executable");
		case PAGE_TARGETS_INVALID:
//			case PAGE_TARGETS_NO_UPDATE:
			return("NO_UPDATE/INVALID");
		default:
			return("None");
	}
}

bool listProcessHeaps(uint32_t pid, int type)
{
	HEAPLIST32 hl;
	HANDLE hHeapSnap = INVALID_HANDLE_VALUE;

	if( !getProcessHeapListSnapshot(&hHeapSnap, &hl, pid) )
	{
		return false;
	}

	printf("List of Heaps:\n");
	printf(" : [flags | heap id | pid]\n");
	do
	{
		printf("Heap: %s | 0x%llx | %lu\n", getHLFlagString(hl.dwFlags), hl.th32HeapID, hl.th32ProcessID);

		if ( type == 2 )
			listProcessHeapBlocks(pid, hl.th32HeapID);

		hl.dwSize = sizeof(HEAPLIST32);
	}
	while (Heap32ListNext(hHeapSnap, &hl));

	printf("\n");

	CloseHandle(hHeapSnap);

	return true;
}

bool getProcessHeapListSnapshot(HANDLE* hHeapSnap, HEAPLIST32* hl, DWORD pid)
{
	if ( !openSnap(hHeapSnap, pid, TH32CS_SNAPHEAPLIST) )
		return false;

	hl->dwSize = sizeof(HEAPLIST32);

	if( !Heap32ListFirst((*hHeapSnap), hl) )
	{
		printf("Cannot list first heap (%lu)\n", GetLastError());
		return false;
	}
	return true;
}

void listProcessHeapBlocks(uint32_t pid, ULONG_PTR base)
{
	uint32_t heap_size = 0;
	HEAPENTRY32 he;
	ZeroMemory(&he, sizeof(HEAPENTRY32));
	he.dwSize = sizeof(HEAPENTRY32);

	if ( Heap32First(&he, pid, base) )
	{
		printf(" - [%17s | %-18s | %11s | %8s | %s | %s | %11s | %18s]\n",
			"hHandle", "Address", "BlockSize", "Flags", "#locks", "Resvd", "processId", "HeapId");
		do
		{
			heap_size += he.dwBlockSize;
			printf(" - 0x%p | 0x%016llx | 0x%9lx | %8s |  %5lu |  %4lu |  0x%8lx | 0x%llx \n",
				   he.hHandle, he.dwAddress, he.dwBlockSize, getHEFlagString(he.dwFlags), he.dwLockCount, he.dwResvd,
				   he.th32ProcessID, he.th32HeapID);

			he.dwSize = sizeof(HEAPENTRY32);
		} while ( Heap32Next(&he) );
		printf(" - - heap_size: 0x%x (%u)\n", heap_size, heap_size);
	}
}

BOOL stackTrace(uint32_t pid)
{
	HANDLE process;

	if ( !openProcess(&process, pid) )
		return FALSE;

	return TRUE;
}

char* getHLFlagString(DWORD flag)
{
	if ( flag == HF32_DEFAULT )
		return "HF32_DEFAULT";
	else
		return "HF32_NONE";
}

char* getHEFlagString(DWORD flag)
{
	switch ( flag )
	{
		case LF32_FIXED:
			return "FIXED";
		case LF32_FREE:
			return "FREE";
		case LF32_MOVEABLE:
			return "MOVEABLE";
		default:
			return "NONE";
	}
}

bool listRunningProcesses()
{
	HANDLE snap;
	HANDLE process;
	PROCESSENTRY32 pe32;
	DWORD dwPriorityClass;
	bool readable;

	if ( !openSnap(&snap, 0, TH32CS_SNAPPROCESS) )
		return false;

	pe32.dwSize = sizeof( PROCESSENTRY32 );
	if( !Process32First(snap, &pe32))
	{
		printf("ERROR: Process32First\n");
		CloseHandle(snap);
		return false;
	}

	printf("List of processes\n");
	printf("%-10s | %-10s | %s | %s | %s |  %s | %s\n", "pid", "ppid", "threads", "pcPriClassBase", "dwPriorityClass", "readable", "name");
	do
	{
		dwPriorityClass = 0;
		readable = false;
		process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
		if ( process )
		{
			dwPriorityClass = GetPriorityClass(process);
//			if ( !dwPriorityClass )
//				printf("ERROR: GetPriorityClass\n");
			CloseHandle(process);
			readable = true;
		}
//		else
//			printf("ERROR: OpenProcess\n");

		printf("0x%08lx | 0x%08lx | %7lu | %14lu | %15lu | %8s | %s\n",
				pe32.th32ProcessID, pe32.th32ParentProcessID, pe32.cntThreads, pe32.pcPriClassBase,
				dwPriorityClass, (readable)?"true":"false", pe32.szExeFile);
	}
	while (Process32Next(snap, &pe32));
	printf("\n");

	CloseHandle(snap);
	return true;
}
