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

int printModuleProcessMemory(HANDLE process, MODULEENTRY32* me32, uint64_t base_off);
size_t readProcessBlock(BYTE* base_addr, DWORD base_size, uint64_t base_off, HANDLE process, unsigned char* block);
bool confirmContinueWithNextModule(MODULEENTRY32* me32);
size_t printProcessBlock(BYTE* base_addr, DWORD base_size, HANDLE process, uint64_t base_off, unsigned char* buffer);
void printError(LPTSTR lpszFunction, DWORD last_error);
BOOL getModule(uint64_t address, HANDLE snap, MODULEENTRY32* me32);
uint64_t findNeedleInProcessMemoryBlock(BYTE* base_addr, DWORD base_size, uint64_t offset, HANDLE process, const unsigned char* needle, uint32_t needle_ln);
BOOL openSnapAndME(HANDLE* snap, MODULEENTRY32* me32, uint32_t pid, DWORD dwFlags);
BOOL openSnap(HANDLE* snap, uint32_t pid, DWORD dwFlags);
BOOL openME(HANDLE* snap, MODULEENTRY32* me32);
BOOL openProcessAndSnapAndME(HANDLE* process, HANDLE* snap, MODULEENTRY32* me32, uint32_t pid, DWORD dwFlags);
void initME32(MODULEENTRY32* me32);
BOOL addressIsInModuleRange(uint64_t address, MODULEENTRY32* me32);
void ProcessHandler_cleanUp(HANDLE snap, HANDLE process);
BOOL openProcess(HANDLE* process, uint32_t pid);
char* getHEFlagString(DWORD flag);

unsigned char* p_needle = NULL;
uint32_t p_needle_ln;

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

uint8_t makeStartAndLengthHitAModule(uint32_t pid, uint64_t* start)
{
	HANDLE snap = INVALID_HANDLE_VALUE;
	MODULEENTRY32 me32;
	uint8_t info_line_break = 0;

	if ( !openSnapAndME(&snap, &me32, pid, TH32CS_SNAPMODULE) )
		return 2;

	do
	{
		if ( addressIsInModuleRange(*start, &me32) )
		{
			if ( length > me32.modBaseSize )
			{
				printf("Info: Length %llu does not fit in module!\nSetting it to %lu!", (*start), me32.modBaseSize);
				length = me32.modBaseSize;
				return 1;
			}
			return 0;
		}
	} while ( Module32Next(snap, &me32) );

	Module32First(snap, &me32);
	CloseHandle(snap);

	if ( (*start) > 0 )
	{
		printf("Info: Start offset %llx does not hit a module!\nSetting it to %p!", (*start), me32.modBaseAddr);
		info_line_break = 1;
	}
	(*start) = (uint64_t) me32.modBaseAddr;

	return info_line_break;
}

BOOL listProcessModules(uint32_t pid)
{
	uint16_t i = 0;
	HANDLE snap = INVALID_HANDLE_VALUE;
	MODULEENTRY32 me32;

	if ( !openSnapAndME(&snap, &me32, pid, TH32CS_SNAPMODULE))
		return FALSE;

	printf("List of modules:\n");
	printf("[Process ID | Ref count (g) | Ref count (p) | Base address | Base size]\n");
	do
	{
		i++;

		printf("%u. Module: %s\n", i, me32.szModule);
		printf(" - Path = %s\n", me32.szExePath);
		printf(" - 0x%08lX |", me32.th32ProcessID);
		printf(" 0x%04lX |", me32.GlblcntUsage);
		printf(" 0x%04lX |", me32.ProccntUsage);
		printf(" 0x%p |", me32.modBaseAddr);
		printf(" %lu\n", me32.modBaseSize);
	}
	while ( Module32Next(snap, &me32));
	printf("\n");

	CloseHandle(snap);

	return TRUE;
}

void initME32(MODULEENTRY32* me32)
{
	memset(me32, 0, sizeof(MODULEENTRY32));
	me32->dwSize = sizeof(MODULEENTRY32);
}

BOOL listProcessThreads(uint64_t pid)
{
	HANDLE hThreadSnap = INVALID_HANDLE_VALUE;
	THREADENTRY32 te32;
	uint16_t i = 0;

	if ( !openSnap(&hThreadSnap, pid, TH32CS_SNAPTHREAD) )
		return FALSE;

	memset(&te32, 0, sizeof(THREADENTRY32));
	te32.dwSize = sizeof(THREADENTRY32);

	if ( !Thread32First(hThreadSnap, &te32) )
	{
		printf("Error: Thread32First\n");
		CloseHandle(hThreadSnap);
		return (FALSE);
	}

	printf("List of threads:\n");
	printf("[Base priority | Delta priority]\n");
	do
	{
		if ( te32.th32OwnerProcessID == pid )
		{
			i++;

			printf("%u. Thread ID: 0x%08lX\n", i, te32.th32ThreadID);
			printf(" - %lu |", te32.tpBasePri);
			printf(" %lu\n", te32.tpDeltaPri);
		}
	}
	while ( Thread32Next(hThreadSnap, &te32));
	printf("\n");

	CloseHandle(hThreadSnap);
	return TRUE;
}

int writeProcessMemory(uint32_t pid, unsigned char* payload, uint32_t payload_ln, uint64_t start)
{
	HANDLE snap = INVALID_HANDLE_VALUE;
	MODULEENTRY32 me32;
	HANDLE process;
	uint64_t base_off = start;
	BOOL s;
	uint8_t i;
	SIZE_T bytes_written = 0;
	DWORD last_error;
	DWORD lpflOldProtect;

	if ( !openProcessAndSnapAndME(&process, &snap, &me32, pid, TH32CS_SNAPMODULE))
		return 1;

	s = getModule(start, snap, &me32);
	if ( !s )
	{
		printf("Error: No module hit for writing\n");
		return 2;
	}

	printf("Write into %s:\n", me32.szModule);
	printf("payload_ln %u\n", payload_ln);
	for ( i = 0; i < payload_ln; i++ )
		printf("%02x|", payload[i]);
	printf("\n");

	base_off -= (uint64_t) me32.modBaseAddr;
	s = VirtualProtectEx(process, me32.modBaseAddr + base_off, payload_ln, PAGE_EXECUTE_READWRITE, &lpflOldProtect);
	if ( !s )
	{
		last_error = GetLastError();
		printf(" - Error: VirtualProtect at 0x%llx\n", (uint64_t) me32.modBaseAddr + base_off);
		printf(" - - code: %lu\n", last_error);
		printError("VirtualProtect", last_error);
	}

	s = WriteProcessMemory(process, me32.modBaseAddr + base_off, payload, payload_ln, &bytes_written);

	if ( !s )
	{
		last_error = GetLastError();
		printf(" - Error: WriteProcessMemory %lu bytes at 0x%p\n", bytes_written, me32.modBaseAddr + base_off);
		printf(" - - code: %lu\n", last_error);
		printError("WriteProcessMemory", last_error);
	}

	s = VirtualProtectEx(process, me32.modBaseAddr + base_off, payload_ln, lpflOldProtect, &lpflOldProtect);
	if ( !s )
	{
		last_error = GetLastError();
		printf(" - Error: VirtualProtect at 0x%llx\n", (uint64_t) me32.modBaseAddr + base_off);
		printf(" - - code: %lu\n", last_error);
		printError("VirtualProtect", last_error);
	}

	ProcessHandler_cleanUp(snap, process);
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
BOOL printProcessModules(uint32_t pid, uint64_t start, uint8_t skip_bytes, unsigned char* needle, uint32_t needle_ln)
{
	MODULEENTRY32 me32;
	HANDLE snap = INVALID_HANDLE_VALUE;
	HANDLE process;
	BOOL s;

	uint64_t base_off;
	uint64_t found = FIND_FAILURE;

	p_needle = needle;
	p_needle_ln = needle_ln;

	if ( !openProcessAndSnapAndME(&process, &snap, &me32, pid, TH32CS_SNAPMODULE))
		return FALSE;

	// find module hit by start address
	s = getModule(start, snap, &me32);
	if ( !s )
	{
		printf("Error: No module hit!\n");
		return FALSE;
	}

	if ( find_f )
		Finder_initFailure(p_needle, p_needle_ln);

	base_off = start - (uint64_t) me32.modBaseAddr;

	do
	{
		if ( find_f )
		{
			found = findNeedleInProcessMemoryBlock(me32.modBaseAddr, me32.modBaseSize, base_off, process, p_needle,
												   p_needle_ln);
			if ( found == FIND_FAILURE )
			{
				s = Module32Next(snap, &me32);
				continue;
			}
			else
			{
				Printer_setHiglightBytes(p_needle_ln);
				base_off = found - (uint64_t) me32.modBaseAddr;
				base_off = normalizeOffset(base_off, &skip_bytes);
				Printer_setHiglightWait(skip_bytes);
			}
		}

//		Printer_setSkipBytes(skip_bytes);
		printModuleProcessMemory(process, &me32, base_off);

		s = Module32Next(snap, &me32);
		if ( !s )
			break;
		if ( !confirmContinueWithNextModule(&me32) )
			break;

		base_off = 0;
	}
	while ( s );

	ProcessHandler_cleanUp(snap, process);
	Finder_cleanUp();

	return TRUE;
}

bool confirmContinueWithNextModule(MODULEENTRY32* me32)
{
	char input;

	printf("\n");
	printf("Continue with next module: %s (c/q)?\n", me32->szModule);
	while ( 1 )
	{
		input = _getch();
		if ( input == 'c' )
			return true;
		else if ( input == 'q')
			return false;
	}
}

/**
 *
 * @param process
 * @param me32
 * @param base_off uint32_t base offset in module to start at printing
 * @return
 */
int printModuleProcessMemory(HANDLE process, MODULEENTRY32* me32, uint64_t base_off)
{
	printf("%s (0x%p - 0x%p):\n", me32->szModule, me32->modBaseAddr, me32->modBaseAddr+me32->modBaseSize);

	size_t n_size = length;
	unsigned char buffer[BLOCKSIZE_LARGE] = {0};
	char input;
	uint64_t found;

	found = base_off;

	n_size = printProcessBlock(me32->modBaseAddr, me32->modBaseSize, process, base_off, buffer);
	base_off += n_size;

	if ( continuous_f )
	{
		while ( n_size && n_size == length )
		{
			input = _getch();

			if ( input == ENTER )
				n_size = printProcessBlock(me32->modBaseAddr, me32->modBaseSize, process, base_off, buffer);
			else if ( find_f && input == 'n' )
			{
				found = findNeedleInProcessMemoryBlock(me32->modBaseAddr, me32->modBaseSize, found + p_needle_ln,
													   process,
													   p_needle, p_needle_ln);
				if ( found == FIND_FAILURE )
					break;
				found -= (uint64_t) me32->modBaseAddr;
				printf("\n");
				Printer_setHiglightBytes(p_needle_ln);
				n_size = printProcessBlock(me32->modBaseAddr, me32->modBaseSize, process, found, buffer);
			}
			else if ( input == 'q' )
				break;

			base_off += n_size;
		}
	}

	return 0;
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
printProcessBlock(BYTE* base_addr, DWORD base_size, HANDLE process, uint64_t base_off, unsigned char* buffer)
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
		if ( addressIsInModuleRange(address, me32) )
			return TRUE;
	}
	while ( Module32Next(snap, me32) );
	return FALSE;
}

BOOL addressIsInModuleRange(uint64_t address, MODULEENTRY32* me32)
{
	uint64_t base = (uint64_t) me32->modBaseAddr;
	uint64_t end = base + me32->modBaseSize;
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
findNeedleInProcessMemoryBlock(BYTE* base_addr, DWORD base_size, uint64_t offset, HANDLE process,
							   const unsigned char* needle,
							   uint32_t needle_ln)
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
		printf("Error (%lu): CreateToolhelp32Snapshot (of modules)\n", GetLastError());
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

bool listProcessMemory(uint32_t pid)
{
	unsigned char *p = NULL;
	MEMORY_BASIC_INFORMATION info;
	HANDLE process = NULL;
	uint64_t usage;

	char f_path[512];
	char* file_name;
	DWORD nSize = 512;

	if ( !openProcess(&process, pid) )
		return false;

	printf("[Name | BaseAddress | AllocationBase | RegionSize | State | Type | Protect | AllocationProtect [| info]]\n");
	for ( p = NULL;
		  VirtualQueryEx(process, p, &info, sizeof(info)) == sizeof(info);
		  p += info.RegionSize )
	{
		usage = 0;

		memset(f_path, 0, 512);
		GetModuleFileNameExA(process, info.AllocationBase, f_path, nSize);
		getFileNameL(f_path, &file_name);
		printf("%s : ", file_name);
		printf("0x%p\t0x%p\t(%6luK)\t", info.BaseAddress, info.AllocationBase, info.RegionSize/1024);

		switch (info.State) {
			case MEM_COMMIT:
				printf("Committed");
				break;
			case MEM_RESERVE:
				printf("Reserved");
				break;
			case MEM_FREE:
				printf("Free    ");
				break;
			default:
				printf("None    ");
		}
		printf("\t");
		switch (info.Type) {
			case MEM_IMAGE:
				printf("Code Module");
				break;
			case MEM_MAPPED:
				printf("Mapped     ");
				break;
			case MEM_PRIVATE:
				printf("Private    ");
			default:
				printf("None       ");
		}
		printf("\t");

		if ((info.State == MEM_COMMIT) && (info.Type == MEM_PRIVATE))
			usage +=info.RegionSize;

		int guard = 0, nocache = 0;

		if ( info.AllocationProtect & PAGE_NOCACHE)
			nocache = 1;
		if ( info.AllocationProtect & PAGE_GUARD )
			guard = 1;

		switch (info.Protect) {
			case PAGE_READONLY:
				printf("Read Only         ");
				break;
			case PAGE_READWRITE:
				printf("Read/Write        ");
				break;
			case PAGE_WRITECOPY:
				printf("Copy on Write     ");
				break;
			case PAGE_EXECUTE:
				printf("Execute only      ");
				break;
			case PAGE_EXECUTE_READ:
				printf("Execute/Read      ");
				break;
			case PAGE_EXECUTE_READWRITE:
				printf("Execute/Read/Write");
				break;
			case PAGE_EXECUTE_WRITECOPY:
				printf("COW Executable    ");
				break;
			case PAGE_NOACCESS:
				printf("NOACCESS          ");
				break;
			case PAGE_TARGETS_INVALID:
//			case PAGE_TARGETS_NO_UPDATE:
				printf("NO_UPDATE/INVALID ");
				break;
			default:
				printf("None              ");
		}

		info.AllocationProtect &= ~(PAGE_GUARD | PAGE_NOCACHE);

		switch (info.AllocationProtect) {
			case PAGE_READONLY:
				printf("Read Only         ");
				break;
			case PAGE_READWRITE:
				printf("Read/Write        ");
				break;
			case PAGE_WRITECOPY:
				printf("Copy on Write     ");
				break;
			case PAGE_EXECUTE:
				printf("Execute only      ");
				break;
			case PAGE_EXECUTE_READ:
				printf("Execute/Read      ");
				break;
			case PAGE_EXECUTE_READWRITE:
				printf("Execute/Read/Write");
				break;
			case PAGE_EXECUTE_WRITECOPY:
				printf("COW Executable    ");
				break;
			case PAGE_NOACCESS:
				printf("NOACCESS          ");
				break;
			case PAGE_TARGETS_INVALID:
//			case PAGE_TARGETS_NO_UPDATE:
				printf("NO_UPDATE/INVALID ");
				break;
			default:
				printf("None              ");
		}

		if (guard)
			printf("\tguard page");
		if (nocache)
			printf("\tnon-cachable");
//		printf("usage: %lu\n", usage);
		printf("\n");
	}
	printf("\n");

//	int i;
//	uint64_t addr = 0x000001CE38B93460;
//	unsigned char* ptr = (unsigned char*) 0x000001CE38B93460;
//	printf("data of 0x%llx:  ", addr);
//	for ( i = 0; i < 4; i++ )
//		printf("0x%02x", ptr[i]);

	return true;
}

bool listProcessHeaps(uint32_t pid)
{
	HEAPLIST32 hl;
	HANDLE hHeapSnap = INVALID_HANDLE_VALUE;
	HANDLE process = NULL;
//	uint32_t i;
//	BOOL s;
//	unsigned char* ptr = NULL;
//	SIZE_T bytes_read = 0;
//	SIZE_T n_size;
//	unsigned char block[BLOCKSIZE_LARGE];
	uint32_t heap_size;

	if ( !openProcess(&process, pid) )
		return false;

	if ( !openSnap(&hHeapSnap, pid, TH32CS_SNAPHEAPLIST) )
		return false;

	hl.dwSize = sizeof(HEAPLIST32);

	if( !Heap32ListFirst(hHeapSnap, &hl) )
		printf ("Cannot list first heap (%lu)\n", GetLastError());

	printf("List of Heaps:\n");
	printf("[flags | heap id | pid]\n");
	do
	{
		printf("Heap %lu | 0x%llx | %lu\n", hl.dwFlags, hl.th32HeapID, hl.th32ProcessID);
//		ptr = (unsigned char*) hl.th32HeapID;

		HEAPENTRY32 he;
		ZeroMemory(&he, sizeof(HEAPENTRY32));
		he.dwSize = sizeof(HEAPENTRY32);
		heap_size = 0;

		if( Heap32First( &he, pid, hl.th32HeapID ) )
		{
			printf(" - [hHandle | dwAddress | dwBlockSize | dwFlags | fwLockCount | dwResvd | th32ProcessId | th32HeapId]\n");
			do
			{
				heap_size += he.dwBlockSize;
				printf( "- 0x%p | 0x%p | 0x%lx | %s |  %lu |  %lu |  %lu | 0x%llx \n",
						he.hHandle, he.dwAddress, he.dwBlockSize, getHEFlagString(he.dwFlags), he.dwLockCount, he.dwResvd, he.th32ProcessID, he.th32HeapID);

//				memset(&block, 0, BLOCKSIZE_LARGE);
//				n_size = (he.dwBlockSize < BLOCKSIZE_LARGE ) ? he.dwBlockSize : BLOCKSIZE_LARGE;
//				s = ReadProcessMemory(process, ptr+heap_size, block, n_size, &bytes_read);
//				if ( s )
//				{
//					printf(" - data: ");
//					for ( i = 0; i < bytes_read; i++ )
//					{
//						printf("%02x|", block[i]);
//					}
//					printf("\n");
//				}
//				else
//					printf("ERROR: reading block failed\n");


				he.dwSize = sizeof(HEAPENTRY32);
			} while( Heap32Next(&he) );
		}
		printf(" - - heap_size: 0x%lx (%lu)\n", heap_size, heap_size);

		hl.dwSize = sizeof(HEAPLIST32);

//		s = VirtualProtectEx(process, hl.th32HeapID, hl.dwSize, lpflOldProtect, &lpflOldProtect);
	}
	while (Heap32ListNext( hHeapSnap, &hl ));

	printf("\n");

	CloseHandle(hHeapSnap);

	return 0;
}

char* getHEFlagString(DWORD flag)
{
	switch ( flag )
	{
		case LF32_FIXED:
			return "LF32_FIXED";
		case LF32_FREE:
			return "LF32_FREE";
		case LF32_MOVEABLE:
			return "LF32_MOVEABLE";
		default:
			return "NONE";
	}
}