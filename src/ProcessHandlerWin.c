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
#include "Globals.h"
#include "Printer.h"
#include "Finder.h"
#include "utils/Helper.h"

typedef int (*MemInfoCallback)(HANDLE, MEMORY_BASIC_INFORMATION*);

size_t getModuleSize(unsigned char* p, MEMORY_BASIC_INFORMATION* info, HANDLE process);
size_t readProcessBlock(BYTE* base_addr, DWORD base_size, size_t base_off, HANDLE process, unsigned char* block);
BOOL getNextPrintableRegion(HANDLE process, MEMORY_BASIC_INFORMATION* info, unsigned char** p, char* file_name, int print_s, PVOID last_base);
BOOL queryNextRegion(HANDLE process, unsigned char** p, MEMORY_BASIC_INFORMATION* info);
BOOL queryNextAccessibleRegion(HANDLE process, unsigned char** p, MEMORY_BASIC_INFORMATION* info);
BOOL queryNextAccessibleBaseRegion(HANDLE process, unsigned char** p, MEMORY_BASIC_INFORMATION* info);
size_t printMemoryBlock(HANDLE process, BYTE* base_addr, size_t base_off, DWORD base_size, unsigned char* buffer);
size_t findNeedleInProcessMemoryBlock(BYTE* base_addr, DWORD base_size, size_t offset, HANDLE process, const unsigned char* needle, uint32_t needle_ln);
BOOL openSnapAndME(HANDLE* snap, MODULEENTRY32* me32, uint32_t pid, DWORD dwFlags);
BOOL openSnap(HANDLE* snap, uint32_t pid, DWORD dwFlags);
BOOL openME(HANDLE* snap, MODULEENTRY32* me32);
BOOL openProcessAndSnapAndME(HANDLE* process, HANDLE* snap, MODULEENTRY32* me32, uint32_t pid, DWORD dwFlags);
void initME32(MODULEENTRY32* me32);
BOOL addressIsInRegionRange(size_t address, size_t base, DWORD size);
void ProcessHandler_cleanUp(HANDLE snap, HANDLE process);
BOOL openProcess(HANDLE* process, uint32_t pid);
Bool getProcessHeapListSnapshot(HANDLE* hHeapSnap, HEAPLIST32* hl, DWORD pid);
void listProcessHeapBlocks(uint32_t pid, ULONG_PTR base);
char* getHEFlagString(DWORD flag);
char* getHLFlagString(DWORD flag);
char* getProtectString(DWORD protect);
int printMemoryInfo(HANDLE process, MEMORY_BASIC_INFORMATION* info);
int iterateProcessMemory(HANDLE process, MEMORY_BASIC_INFORMATION* info, MemInfoCallback cb);
char* getMemoryStateString(DWORD state);
char* getMemoryTypeString(DWORD type);
int printRegionProcessMemory(HANDLE process, BYTE* base_addr, size_t base_off, SIZE_T size, size_t found, char* reg_name);
BOOL getRegion(size_t address, HANDLE process, MEMORY_BASIC_INFORMATION* info, unsigned char* info_p);
BOOL getRegionName(HANDLE process, PVOID base, char* file_name);
BOOL notAccessibleRegion(MEMORY_BASIC_INFORMATION* info);
BOOL isAccessibleRegion(MEMORY_BASIC_INFORMATION* info);
BOOL setRegionProtection(HANDLE process, MEMORY_BASIC_INFORMATION* info, DWORD new_protect, DWORD* old_protect);
BOOL keepLengthInModule(unsigned char* p, MEMORY_BASIC_INFORMATION* info, HANDLE process, size_t start, size_t* length);
void printModuleRegionInfo(MEMORY_BASIC_INFORMATION* info, const char* file_name, unsigned char* p, HANDLE process);
void printRunningProcessInfo(PROCESSENTRY32* pe32);

static unsigned char* p_needle = NULL;
static uint32_t p_needle_ln;

static HANDLE hStdout;
static WORD wOldColorAttrs;
static HANDLE hStdout;
static CONSOLE_SCREEN_BUFFER_INFO csbiInfo;

size_t getSizeOfProcess(uint32_t pid)
{
	size_t p_size = 0;
	HANDLE snap = INVALID_HANDLE_VALUE;
	MODULEENTRY32 me32;

	if ( !openSnapAndME(&snap, &me32, pid, TH32CS_SNAPMODULE) )
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
 * @param start size_t*
 * @return uint8_t if there has been error output, flags a line break
 */
uint8_t makeStartAndLengthHitAccessableMemory(uint32_t pid, size_t* start)
{
	unsigned char *p = NULL;
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
		
		base_addr = info.BaseAddress;
		if ( *start < (size_t) base_addr )
			break;

		if ( addressIsInRegionRange(*start, (size_t) base_addr, info.RegionSize) )
		{
			info_line_break = keepLengthInModule(p, &info, process, *start, &length);
			CloseHandle(process);
			return info_line_break;
		}
	}

	if ( p == NULL )
	{
		CloseHandle(process);
		return 2;
	}

	if ( VirtualQueryEx(process, p, &info, sizeof(info)) != sizeof(info) )
	{
		CloseHandle(process);
		return 3;
	}

	if ( (*start) > 0 )
	{
		printf("Info: Start offset 0x%llx does not hit a module!\nSetting it to 0x%p!\n", (*start), info.AllocationBase);
		info_line_break = 1;
	}
	(*start) = (size_t) info.AllocationBase;
	if ( keepLengthInModule(p, &info, process, *start, &length))
		info_line_break = 1;

	CloseHandle(process);
	
	return info_line_break;
}

/**
 * Keep a given length in a hit module.
 * 
 * @param info MEMORY_BASIC_INFORMATION* the module info
 * @param start size_t the start offset
 * @param length size_t the length
 * @return BOOL flags, if length has been modified
 */
BOOL keepLengthInModule(unsigned char* p, MEMORY_BASIC_INFORMATION* info, HANDLE process, size_t start, size_t* length)
{
	DWORD base_off = start - (size_t) info->BaseAddress;
	size_t module_size = getModuleSize(p, info, process);
	
	if ( base_off + *length > module_size )
	{
		printf("Info: Length 0x%llx does not fit in region!\nSetting it to 0x%llx!\n", *length, module_size - base_off);
		*length = module_size - base_off;
		return TRUE;
	}
	
	return FALSE;
}

/**
 * Calculate size of Module, i.e. continuous regions with the same name.
 * 
 * @param info 
 * @param process 
 * @return 
 */
size_t getModuleSize(unsigned char* p, MEMORY_BASIC_INFORMATION* info, HANDLE process)
{
	size_t module_size = info->RegionSize;
	size_t module_base = (uintptr_t) info->AllocationBase;

	for ( p += info->RegionSize; // skip one region, cause it's already handled
		  VirtualQueryEx(process, p, info, sizeof(*info)) == sizeof(*info);
		  p += info->RegionSize )
	{
		if ( info->Protect == PAGE_NOACCESS || info->Protect == 0 )
			continue;

		if ( module_base != (uintptr_t) info->AllocationBase )
			break;

		module_size += info->RegionSize;
	}

	return module_size;
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
		printf(" 0x%lx\n", me32.modBaseSize);
	}
	while ( Module32Next(snap, &me32) );
	printf("\n");

	CloseHandle(snap);

	return TRUE;
}

BOOL listProcessThreads(size_t pid)
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

int writeProcessMemory(uint32_t pid, unsigned char* payload, uint32_t payload_ln, size_t start)
{
	HANDLE process;
	MEMORY_BASIC_INFORMATION info;
	unsigned char* info_p = NULL;

	PVOID base_addr = (PVOID) start;
	BOOL s;
	SIZE_T bytes_written = 0;
	DWORD last_error;
	DWORD old_protect;

	const DWORD nSize = 512;
	char f_path[512];
	char* file_name;
	memset(f_path, 0, 512);

	if ( !openProcess(&process, pid) )
		return 1;

	s = getRegion(start, process, &info, info_p);
	if ( !s )
	{
		printf("Error: No region hit for writing\n");
		CloseHandle(process);
		return 2;
	}

	GetModuleFileNameExA(process, info.AllocationBase, f_path, nSize);
	getFileNameL(f_path, &file_name);

	s = VirtualProtectEx(process, base_addr, payload_ln, PAGE_EXECUTE_READWRITE, &old_protect);
	if ( !s )
	{
		last_error = GetLastError();
		printf(" - Error (0x%lx): VirtualProtect at 0x%p\n", last_error, base_addr);
	}

	s = WriteProcessMemory(process, base_addr, payload, payload_ln, &bytes_written);

	if ( !s )
	{
		last_error = GetLastError();
		printf(" - Error (0x%lx): WriteProcessMemory %llu bytes at 0x%p\n", last_error, bytes_written, base_addr);
//		printError("WriteProcessMemory", last_error);
	}

	s = VirtualProtectEx(process, base_addr, payload_ln, old_protect, &old_protect);
	if ( !s )
	{
		last_error = GetLastError();
		printf(" - Error (0x%lx): VirtualProtect at 0x%p\n", last_error, base_addr);
//		printError("VirtualProtect", last_error);
	}

	CloseHandle(process);

	return 0;
}

/**
 *
 * @param pid
 * @param start size_t absolute start offset
 * @param skip_bytes
 * @param needle
 * @param needle_ln
 * @return
 */
BOOL printProcessRegions(uint32_t pid, size_t start, uint8_t skip_bytes, unsigned char* needle, uint32_t needle_ln)
{
	HANDLE process;
	MEMORY_BASIC_INFORMATION info;
	BOOL s;
	int print_s = 0;
	unsigned char *info_p = NULL;
	PVOID last_base = 0;
	DWORD old_protect = 0;

	size_t base_off;
	size_t found = FIND_FAILURE;

	char file_name[PATH_MAX] = {0};

	p_needle = needle;
	p_needle_ln = needle_ln;

	if ( !openProcess(&process, pid) )
		return FALSE;

	s = getRegion(start, process, &info, info_p);
	if ( !s )
	{
		printf("ERROR: No region hit!\n");
		CloseHandle(process);
		return FALSE;
	}

	if ( find_f )
		Finder_initFailure(p_needle, p_needle_ln);

	getRegionName(process, info.AllocationBase, file_name);
	info_p = info.BaseAddress;
	last_base = info.AllocationBase;
//	base_off = start - (size_t) info.AllocationBase; // ?? why not info.BaseAddress ??
	base_off = start - (size_t) info.BaseAddress; // ?? why not info.BaseAddress ??
	printModuleRegionInfo(&info, file_name, info_p, process);

	while ( s )
	{
		setRegionProtection(process, &info, PAGE_READONLY, &old_protect);

		if ( find_f )
		{
			found = findNeedleInProcessMemoryBlock(info.BaseAddress, info.RegionSize, base_off, process, p_needle,
												   p_needle_ln);
			if ( found == FIND_FAILURE )
			{
				if ( !getNextPrintableRegion(process, &info, &info_p, file_name, print_s, last_base) )
					break;

				last_base = info.AllocationBase;
				continue;
			}
			else
			{
				found = found - (size_t) info.BaseAddress;
				base_off = normalizeOffset(found, &skip_bytes);
				Printer_setHiglightBytes(p_needle_ln);
				Printer_setHiglightWait(skip_bytes);
				skip_bytes = 0;
			}
		}

//		Printer_setSkipBytes(skip_bytes);
		print_s = printRegionProcessMemory(process, info.BaseAddress, base_off, info.RegionSize, found, file_name);

		setRegionProtection(process, &info, old_protect, &old_protect);

		if ( !getNextPrintableRegion(process, &info, &info_p, file_name, print_s, last_base) )
			break;

		if ( print_s == 1 )
			printModuleRegionInfo(&info, file_name, NULL, NULL);
		last_base = info.AllocationBase;
		base_off = 0;
	}

	CloseHandle(process);
	Finder_cleanUp();

	return TRUE;
}

void printModuleRegionInfo(MEMORY_BASIC_INFORMATION* info, const char* file_name, unsigned char* p, HANDLE process)
{
	size_t module_size = getModuleSize(p, info, process);
	printf("%s (0x%p - 0x%p):\n", file_name, (BYTE*) info->AllocationBase, (BYTE*) ((uintptr_t) info->AllocationBase + module_size));
//	printf("%s (0x%p - 0x%p):\n", file_name, (BYTE*) info->AllocationBase, (BYTE*) info->AllocationBase + info->RegionSize);
}

BOOL setRegionProtection(HANDLE process, MEMORY_BASIC_INFORMATION* info, DWORD new_protect, DWORD* old_protect)
{
	BOOL s;
//	if ( strncmp(getProtectString(info->Protect), "None", 6) == 0 )
	{
		s = VirtualProtectEx(process, info->BaseAddress, info->RegionSize, new_protect, old_protect);
		if ( !s )
		{
			printf(" - Error (0x%lx): VirtualProtect at 0x%llx\n", GetLastError(), (size_t) info->BaseAddress);
//			printError("VirtualProtect", GetLastError());
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
		if ( !confirmContinueWithNextRegion(file_name, (uintptr_t) info->AllocationBase) )
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
//		printf("ERROR (0x%08lx) VirtualQueryEx: could not query info!\n", GetLastError());
//		printError("queryNextRegion", GetLastError());
		return FALSE;
	}

	return TRUE;
}

BOOL getRegionName(HANDLE process, PVOID base, char* file_name)
{
	const DWORD f_path_size = PATH_MAX;
	char f_path[PATH_MAX] = {0};
	DWORD s;
//	printf("getRegionName()\n");

//	GetModuleBaseNameA(process, base, f_path, f_path_size); // sometimes not valid
	s = GetMappedFileNameA(process, base, f_path, f_path_size);
//	printf(" - s: %lu\n", s);
	if ( s == 0 )
	{
		file_name[0] = 0;
		return FALSE;
	}
//	printf(" - f_path: %s\n", f_path);
	f_path[f_path_size-1] = 0;
//	printf(" - f_path: %s\n", f_path);
	getFileName(f_path, file_name);
//	printf(" - file_name: %s\n", file_name);

	return TRUE;
}

/**
 *
 * @param process
 * @param me32
 * @param base_off uint32_t base offset in module to start at printing
 * @return 0 if end of block is reached, 1 if forced to quit
 */
int printRegionProcessMemory(HANDLE process, BYTE* base_addr, size_t base_off, SIZE_T size, size_t found, char* reg_name)
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
			found -= (size_t) base_addr;
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
printMemoryBlock(HANDLE process, BYTE* base_addr, size_t base_off, DWORD base_size, unsigned char* buffer)
{
	size_t n_size = readProcessBlock(base_addr, base_size, base_off, process, buffer);
	uint8_t offset_width = countHexWidth64((size_t)base_addr+base_size);
	if ( n_size )
		printLine(buffer, (size_t)base_addr + base_off, n_size, offset_width);

	return n_size;
}

/**
 *
 * @param me32 MODULEENTRY32*
 * @param process HANDLE
 * @param base_off size_t
 * @param block char*
 * @return
 */
size_t readProcessBlock(BYTE* base_addr, DWORD base_size, size_t base_off, HANDLE process, unsigned char* block)
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
		printf(" - Error (0x%lx): ReadProcessMemory 0x%llx bytes at 0x%llx\n", GetLastError(), bytes_read, (size_t)base_addr + base_off);
		return 0;
	}

	return n_size;
}

/**
 * Get accessible region hit by given address.
 *
 * @param address
 * @param process HANDLE
 * @param info MEMORY_BASIC_INFORMATION*
 * @return BOOL
 */
BOOL getRegion(size_t address, HANDLE process, MEMORY_BASIC_INFORMATION* info, unsigned char* info_p)
{
	for ( info_p = NULL;
		  VirtualQueryEx(process, info_p, info, sizeof(*info)) == sizeof(*info);
		  info_p += info->RegionSize )
	{
//		printf(" - check: 0x%p, 0x%p, \n", info->BaseAddress, info->RegionSize);
		if ( !isAccessibleRegion(info) )
			continue;

//		printf(" - - is accessable: 0x%p, 0x%p, 0x%p, \n", address, info->BaseAddress, info->RegionSize);
		if ( addressIsInRegionRange(address, (size_t) info->BaseAddress, info->RegionSize) )
			return TRUE;
	}
	return FALSE;
}

///**
// * Get module hit by address.
// *
// * @param address
// * @param snap
// * @param me32
// * @return
// */
//BOOL getModule(size_t address, HANDLE snap, MODULEENTRY32* me32)
//{
//	do
//	{
//		if ( addressIsInRegionRange(address, (size_t) me32->modBaseAddr, me32->modBaseSize))
//			return TRUE;
//	}
//	while ( Module32Next(snap, me32) );
//	return FALSE;
//}

BOOL addressIsInRegionRange(size_t address, size_t base, DWORD size)
{
	size_t end = base + size;
//	printf(" - - - addressIsInRegionRange: 0x%p, 0x%p, 0x%p, \n", address, base, end);
	return base <= address && address < end;
}

/**
 * Find needle in a process memory block.
 *
 * @param me32
 * @param process
 * @param needle
 * @param needle_ln
 * @param offset
 * @return size_t absolute found address
 */
size_t
findNeedleInProcessMemoryBlock(BYTE* base_addr, DWORD base_size, size_t offset, HANDLE process, const unsigned char* needle, uint32_t needle_ln)
{
	size_t found = FIND_FAILURE;
	size_t block_i;
	size_t j = 0;
	size_t base_off = offset;
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
			found = (size_t) base_addr + base_off + block_i - needle_ln + 1;
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
		printf("ERROR (0x%lx): OpenProcess %u failed\n", GetLastError(), pid);
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
		printf("ERROR (0x%lx): CreateToolhelp32Snapshot\n", GetLastError());
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
		printf("ERROR (0x%lx): Module32First", last_error);
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

Bool listProcessMemory(uint32_t pid)
{
	HANDLE process = NULL;
	MEMORY_BASIC_INFORMATION info;
	int w_rs = (sizeof(size_t) * 2) + 2;

	if ( !openProcess(&process, pid) )
		return false;

	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(hStdout, &csbiInfo);
	wOldColorAttrs = csbiInfo.wAttributes;

	printf("%-*s | %-*s | %*s | %-9s | %-11s | %-18s | %-18s | %s [| %s]\n",
		   w_rs, "Address", w_rs, "Base", w_rs, "Size", "State", "Type", "Protect", "AllocationProtect", "Name", "info");
	printf("----------------------------------------------------------------------------------------------------------------------------------------\n");
	iterateProcessMemory(process, &info, &printMemoryInfo);
	printf("\n");

	return true;
}

int printMemoryInfo(HANDLE process, MEMORY_BASIC_INFORMATION* info)
{
//	size_t usage = 0;
	char f_path[PATH_MAX+1];
	char* file_name;
	DWORD f_path_size = PATH_MAX;
	const char* SEPARATOR = " | ";
	int guard = 0, nocache = 0;
	int w_rs = sizeof(size_t) * 2;

	if ( !isAccessibleRegion(info) )
		SetConsoleTextAttribute(hStdout, FOREGROUND_INTENSITY);

	printf("0x%p | 0x%p | 0x%*lx | ", info->BaseAddress, info->AllocationBase, w_rs, info->RegionSize);

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

	if ( !isAccessibleRegion(info) )
		SetConsoleTextAttribute(hStdout, wOldColorAttrs);

	return 0;
}

BOOL notAccessibleRegion(MEMORY_BASIC_INFORMATION* info)
{
	return ( info->State == MEM_FREE && info->Protect == PAGE_NOACCESS ) ||
			( info->State == MEM_RESERVE && info->Type == MEM_PRIVATE && info->Protect == 0 );
}

BOOL isAccessibleRegion(MEMORY_BASIC_INFORMATION* info)
{
//	return	!( info->State == MEM_FREE && info->Protect == PAGE_NOACCESS )
	return	info->Protect != PAGE_NOACCESS
			&&
			!( info->State == MEM_RESERVE && (info->Type == MEM_PRIVATE||info->Type == MEM_MAPPED) && info->Protect == 0 )
//			&&
//			!( info->State == MEM_RESERVE && info->Type == MEM_MAPPED && info->Protect == 0 )
			;
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

Bool listProcessHeaps(uint32_t pid, int type)
{
	HEAPLIST32 hl;
	HANDLE hHeapSnap = INVALID_HANDLE_VALUE;

	if( !getProcessHeapListSnapshot(&hHeapSnap, &hl, pid) )
	{
		return false;
	}
	
	printf("List of Heaps:\n");
	printf("%-13s | %-*s | %-s\n",  "flags", (sizeof(SIZE_T)*2+2), "heap id", "pid");
	printf("------------------------------------------------\n");
	do
	{
		printf("%-13s | 0x%p | %lu\n", getHLFlagString(hl.dwFlags), (void*)hl.th32HeapID, hl.th32ProcessID);

		if ( type == 2 )
			listProcessHeapBlocks(pid, hl.th32HeapID);

		hl.dwSize = sizeof(HEAPLIST32);
	}
	while (Heap32ListNext(hHeapSnap, &hl));

	printf("\n");

	CloseHandle(hHeapSnap);

	return true;
}

Bool getProcessHeapListSnapshot(HANDLE* hHeapSnap, HEAPLIST32* hl, DWORD pid)
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
			printf(" - 0x%p | 0x%016llx | 0x%9llx | %8s |  %5lu |  %4lu |  0x%8lx | 0x%llx \n",
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

Bool listRunningProcesses()
{
	HANDLE snap;
	PROCESSENTRY32 pe32;

	if ( !openSnap(&snap, 0, TH32CS_SNAPPROCESS) )
		return false;

	pe32.dwSize = sizeof( PROCESSENTRY32 );
	if( !Process32First(snap, &pe32))
	{
		printf("ERROR: Process32First\n");
		CloseHandle(snap);
		return false;
	}

	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(hStdout, &csbiInfo);
	wOldColorAttrs = csbiInfo.wAttributes;

	printf("List of processes\n");
	printf("%-10s | %-10s | %s | %s | %s |  %s | %s\n", "pid", "ppid", "threads", "base priority", "priority", "readable", "name");
	printf("----------------------------------------------------------------------------------------\n");
	do
	{
		printRunningProcessInfo(&pe32);
	}
	while (Process32Next(snap, &pe32));
	printf("\n");

	CloseHandle(snap);
	return true;
}

void printRunningProcessInfo(PROCESSENTRY32* pe32)
{
	HANDLE process;
	DWORD priorityClass = 0;
	Bool readable = false;
	process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, (*pe32).th32ProcessID);
	if ( process )
	{
		priorityClass = GetPriorityClass(process);
//			if ( !priorityClass )
//				printf("ERROR: GetPriorityClass\n");
		CloseHandle(process);
		readable = true;
	}
//		else
//			printf("ERROR: OpenProcess\n");

	if ( !readable )
		SetConsoleTextAttribute(hStdout, FOREGROUND_INTENSITY);
	printf("0x%08lx | 0x%08lx | %7lu | %13lu | %8lu | %9s | %s\n",
		   (*pe32).th32ProcessID, (*pe32).th32ParentProcessID, (*pe32).cntThreads, (*pe32).pcPriClassBase,
		   priorityClass, (readable) ? "true" : "false", (*pe32).szExeFile);
	if ( !readable )
		SetConsoleTextAttribute(hStdout, wOldColorAttrs);
}
