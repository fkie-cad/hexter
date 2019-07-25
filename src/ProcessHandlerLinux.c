#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

#include "ProcessHandlerLinux.h"
#include "utils/common_fileio.h"
#include "utils/Converter.h"
#include "utils/TerminalUtil.h"
#include "Globals.h"
#include "Printer.h"
#include "Finder.h"
#include "Writer.h"
#include "utils/Helper.h"

typedef struct ProcMapsEntry
{
	uint64_t base;
	uint64_t address;
	uint64_t size;
	char perms[5];
	uint64_t offset;
	char dev[6];
	uint32_t inode;
	char pathname[1024];
} ProcMapsEntry;

typedef int (*MapsInfoCallback)(ProcMapsEntry* entry, uint32_t last_module_inode, size_t line_nr);

static bool openProcessFile(uint32_t pid, int *fd, char *type, int flag);
static bool fopenProcessFile(uint32_t pid, FILE **fp, char *type, const char* mode);
static bool openProcessMemory(uint32_t pid, int *fd, int flag);
static bool fopenProcessMemory(uint32_t pid, FILE **fp, const char* mode);
static bool openProcessMaps(uint32_t pid, int *fd);
static bool fopenProcessMaps(uint32_t pid, FILE **fp);

static bool parseProcMapsLine(char* line, ProcMapsEntry* entry);

static int printProcMapEntry(ProcMapsEntry* entry, uint32_t last_module_inode, size_t line_nr);

static bool parseProcessMaps(uint32_t pid, MapsInfoCallback cb);
static void printProcessModulesTableHeader(const uint8_t map_entry_col_width[6]);
static int printProcModule(ProcMapsEntry* entry, uint32_t last_module_inode, size_t module_nr);

static int printProcessHeap(ProcMapsEntry* entry, uint32_t last_module_inode, size_t module_nr);

static bool getProcName(uint32_t pid, char* name);
static bool isReadableRegion(ProcMapsEntry *entry);
static bool addressIsInRegionRange(uint64_t address, uint64_t base, uint64_t size);
void setModuleEndAddress(ProcMapsEntry *entry, FILE *fp);
static uint64_t getModuleEndAddress(ProcMapsEntry *module, FILE* fp);
static bool keepLengthInModule(ProcMapsEntry *entry, uint64_t start, uint64_t *length);

bool getRegion(uint64_t start, FILE* fp, ProcMapsEntry* entry);
void printRegionInfo(ProcMapsEntry* entry, const char* file_name);
bool queryNextRegion(FILE* fp, ProcMapsEntry* entry);
int printRegionProcessMemory(uint32_t pid, uint64_t base_addr, uint64_t base_off, uint64_t size, uint64_t found, int print_s);
uint64_t printMemoryBlock(uint64_t nr_of_parts, unsigned char* block, FILE* fi, uint16_t block_size, uint64_t block_start, uint64_t block_max);

static const uint8_t map_entry_col_width[6] = { 16, 5, 8, 5, 8, 8 };
#define LINE_BUFFER_SPACE 513
static const uint16_t line_size = 512;

static unsigned char* p_needle = NULL;
static uint32_t p_needle_ln;

/**
 * Open proc pid file
 *
 * @param	pid uint32_t the fp id
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
 * @param	pid uint32_t the fp id
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

/**
 * -rw------- ... mem
 */
bool fopenProcessMemory(uint32_t pid, FILE **fp, const char* mode)
{
	return fopenProcessFile(pid, fp, "mem", mode);
}

bool openProcessMaps(uint32_t pid, int *fd)
{
	return openProcessFile(pid, fd, "maps", O_RDONLY);
}

/**
 * -r--r--r-- ... maps
 */
bool fopenProcessMaps(uint32_t pid, FILE **fp)
{
	return fopenProcessFile(pid, fp, "maps", "r");
}

/**
 * Read out fp maps: /proc/pid/maps
 * Format: address perms offset dev inode pathname
 *
 * perms:
 * 	r = read
 * 	w = write
 * 	x = execute
 * 	s = shared
 * 	p = private (copy on write)
 */
bool parseProcessMaps(uint32_t pid, MapsInfoCallback cb)
{
	FILE* fp = NULL;
	const uint16_t line_size = 512;
	char line[513];
	size_t line_nr = 0;
	bool last_module_inode = 0;
	ProcMapsEntry entry;

	if ( !fopenProcessMaps(pid, &fp) )
		return 0;

	while ( fgets(line, line_size, fp) )
	{
		line[line_size] = 0;

		if ( !parseProcMapsLine(line, &entry) )
			break;

		cb(&entry, last_module_inode, line_nr);

		if ( entry.inode != 0 ) last_module_inode = entry.inode;
		line_nr++;
	}

	fclose(fp);

	return true;
}

bool parseProcMapsLine(char* line, ProcMapsEntry* entry)
{
	const uint16_t bucket_max = 8;
	uint16_t bucket_ln = 0;
	char* bucket[8];

	const uint8_t from_to_max = 2;
	uint8_t from_to_ln = 0;
	char* from_to[2];

	int s;
	uint64_t address, size;

	bucket_ln = split(line, " \n", bucket, bucket_max);

	if ( bucket_ln < 1 )
		return false;

	from_to_ln = split(bucket[0], "-", from_to, from_to_max);
	if ( from_to_ln != 2 )
		return false;

	s = parseUint64(from_to[0], &address, 16);
	if ( s != 0 )
		return false;

	s = parseUint64(from_to[1], &size, 16);
	if ( s != 0 )
		return false;
	size = size - address;

	entry->address = address;
	entry->base = address;
	entry->size = size;
//	memset(entry->perms, 0, 5);
	strncpy(entry->perms, bucket[1], 4);
	s = parseUint64(bucket[2], &entry->offset, 16);
	if ( s != 0 )
		return false;

//	memset(entry->dev, 0, 6);
//	memcpy(entry->dev, bucket[3], 5);
	strncpy(entry->dev, bucket[3], 5);
	s = parseUint32(bucket[4], &entry->inode, 10);
	if ( s != 0 )
		return false;

	if ( bucket_ln >= 6 )
	{
//		memset(entry->pathname, 0, 1024);
//		memcpy(entry->pathname, bucket[5], strnlen(bucket[5], 1023));
		strncpy(entry->pathname, bucket[5], 1023);
	}
	else
		entry->pathname[0] = 0;

	entry->perms[4] = 0;
	entry->dev[5] = 0;
	entry->pathname[1023] = 0;

	return true;
}

uint8_t makeStartAndLengthHitAccessableMemory(uint32_t pid, uint64_t *start)
{
	FILE *fp = NULL;
	char line[LINE_BUFFER_SPACE];

	uint8_t info_line_break = 0;

	ProcMapsEntry entry;
	memset(&entry, 0, sizeof(entry));

	if ( !fopenProcessMaps(pid, &fp) )
	{
		printf("ERROR: Could not open fp %u.\n", pid);
		return 0;
	}

	while ( fgets(line, line_size, fp) )
	{
		line[line_size] = 0;

		if ( !parseProcMapsLine(line, &entry) )
			break;

		if ( !isReadableRegion(&entry) )
			continue;

		if ( *start < entry.address )
			break;

		if ( addressIsInRegionRange(*start, entry.address, entry.size) )
		{
			setModuleEndAddress(&entry, fp);
			info_line_break = keepLengthInModule(&entry, *start, &length);
			fclose(fp);
			return info_line_break;
		}
	}
	// entry will be either not set or closest to start
	if ( entry.address == 0 )
	{
		fclose(fp);
		return 2;
	}

	if ( (*start) > 0 )
	{
		printf("Info: Start offset 0x%lx does not hit a module!\nSetting it to 0x%lx!\n", (*start), entry.address);
		info_line_break = 1;
	}
	printf("Info: Start offset 0x%lx does not hit a module!\nSetting it to 0x%lx!\n", (*start), entry.address);
	(*start) = (uint64_t) entry.address;

	setModuleEndAddress(&entry, fp);
	if ( keepLengthInModule(&entry, *start, &length) )
		info_line_break = 1;

	fclose(fp);

	return info_line_break;
}

bool isReadableRegion(ProcMapsEntry *entry)
{
	return entry->perms[0] == 'r';
}

bool addressIsInRegionRange(uint64_t address, uint64_t base, uint64_t size)
{
	uint64_t end = base + size;
	return base <= address && address < end;
}

void setModuleEndAddress(ProcMapsEntry *entry, FILE *fp)
{
	uint64_t end_address = getModuleEndAddress(&*entry, fp);
	entry->size = end_address - entry->address;
}

uint64_t getModuleEndAddress(ProcMapsEntry *module, FILE* fp)
{
	char line[LINE_BUFFER_SPACE];
	ProcMapsEntry entry;
	memset(&entry, 0, sizeof(entry));
	uint64_t end_address = module->address + module->size;

	while ( fgets(line, line_size, fp) )
	{
		line[line_size] = 0;

		if ( !parseProcMapsLine(line, &entry) )
			break;

		// possible breaks in module
		if ( !entry.pathname || entry.pathname[0] == 0 || entry.inode == 0 )
			continue;
		if ( entry.pathname[0] == '[' )
			break;

		if ( entry.inode == module->inode )
			end_address = entry.address + entry.size;
		else
			break;
	}

	return end_address;
}

bool keepLengthInModule(ProcMapsEntry *entry, uint64_t start, uint64_t *length)
{
	uint64_t base_off = start - entry->address;
	if ( base_off + *length > entry->size )
	{
		printf("Info: Length 0x%lx does not fit in region!\nSetting it to 0x%lx!\n", *length, entry->size - base_off);
		*length = entry->size - base_off;
		return true;
	}
	return false;
}

/**
 * Get fp name
 *
 * @param	pid uint32_t
 * @param	name char* allocated space for the name
 */
bool getProcName(uint32_t pid, char* name)
{
	FILE *fp = NULL;
	char line[LINE_BUFFER_SPACE];
	char* tmp_name;
	char* bucket[1];
	size_t bucket_ln;

	// -r--r--r-- ... cmdline
	if ( !fopenProcessFile(pid, &fp, "cmdline", "r") )
	{
		printf("ERROR: failed to open /proc/%u/cmdline\n", pid);
		return false;
	}
	fgets(line, line_size, fp);
	line[line_size] = 0;

	bucket_ln = split(line, " ", bucket, 1);
	if ( bucket_ln == 0 )
		return false;

	getFileNameL(bucket[0], &tmp_name);

	memcpy(name, tmp_name, strnlen(tmp_name, line_size));

	return true;
}

/**
 * Actually gets the size of the main program fp.
 */
uint64_t getSizeOfProcess(uint32_t pid)
{
	FILE *fp = NULL;
	const uint16_t line_size = 512;
	char line[513];

	ProcMapsEntry entry;
	memset(&entry, 0, sizeof(entry));
	ProcMapsEntry last_entry;
	memset(&entry, 0, sizeof(entry));
	uint64_t start_address;

	char* module_name = NULL;
	bool proc_module_started = false;

	char proc_name[512] = {0};
	if ( !getProcName(pid, proc_name) )
		return 0;

	if ( !fopenProcessMaps(pid, &fp) )
		return 0;

	while ( fgets(line, line_size, fp) )
	{
		line[line_size] = 0;

		if ( !parseProcMapsLine(line, &entry) )
			break;

		if ( entry.pathname && entry.pathname[0] == 0 )
			continue;

		getFileNameL(entry.pathname, &module_name);

		if ( strncmp(module_name, proc_name, line_size) != 0 )
		{
			if ( proc_module_started )
				break;
			else
				continue;
		}
		if ( !proc_module_started )
			start_address = entry.address;
		proc_module_started = true;
//		memset(&last_entry, 0, sizeof(last_entry));
		memcpy(&last_entry, &entry, sizeof(entry));
	}

	fclose(fp);

	return last_entry.address + last_entry.size - start_address;
}

/**
 * List all fp modules.
 *
 * @param	pid uint32_t the target fp pid
 * @return	bool success state
 */
bool listProcessModules(uint32_t pid)
{
	FILE *fp = NULL;
	char line[LINE_BUFFER_SPACE];

	uint32_t last_module_inode = 0;
	uint64_t module_size = 0;
	uint64_t module_nr = 0;

	ProcMapsEntry entry;
	memset(&entry, 0, sizeof(entry));
	ProcMapsEntry last_entry;
	memset(&entry, 0, sizeof(entry));
	uint64_t start_address;

	if ( !fopenProcessMaps(pid, &fp) )
		return 0;

	printProcessModulesTableHeader(map_entry_col_width);

	while ( fgets(line, line_size, fp) )
	{
		line[line_size] = 0;

		if ( !parseProcMapsLine(line, &entry) )
			break;

		if ( !entry.pathname || entry.pathname[0] == 0 )
//		if ( !entry.pathname || strnlen(entry.pathname, PATH_MAX) == 0 )
			continue;
		if ( entry.pathname[0] == '[' || entry.inode == 0 )
			continue;


		if ( entry.inode == last_module_inode )
			module_size += entry.size;
		// just happens the first time, a module is hit, otherwise 0 has been already skipped
		else if ( last_module_inode == 0 )
		{
			module_size = entry.size;
			start_address = entry.address;
		}
		else
		{
			module_nr++;
			last_entry.size = module_size;
			last_entry.address = start_address;
			module_size = entry.size;
			start_address = entry.address;
			printProcModule(&last_entry, last_module_inode, module_nr);
		}

		last_module_inode = entry.inode;

//		memset(&last_entry, 0, sizeof(last_entry));
		memcpy(&last_entry, &entry, sizeof(entry));
	}
	module_nr++;
	last_entry.size = module_size;
	last_entry.address = start_address;
	printProcModule(&last_entry, last_module_inode, module_nr);

	printf("\n");

	fclose(fp);

	return true;
}

void printProcessModulesTableHeader(const uint8_t map_entry_col_width[6])
{
	printf("List of modules:\n");
	printf("x. Name\n");
	printf(" - Path\n");
	printf(" - %-*s | %-*s | %-*s | %*s | %*s | %*s\n",
			map_entry_col_width[0] + 2, "address", map_entry_col_width[0] + 2, "size", map_entry_col_width[1], "perms",
			map_entry_col_width[2] + 2, "offset", map_entry_col_width[3], "dev", map_entry_col_width[4], "inode");
	printf("----------------------------------------------------------------------------------\n");
}

int printProcModule(ProcMapsEntry* entry, uint32_t last_module_inode, size_t module_nr)
{
	char* name = NULL;
	getFileNameL(entry->pathname, &name);

	printf("%lu. Module: %s\n", module_nr, name);
	printf(" - Path = %s\n", entry->pathname);
	printf(" - 0x%0*lx | 0x%0*lx | %*s | 0x%0*lx | %*s | %*u\n",
			map_entry_col_width[0], entry->address, map_entry_col_width[0], entry->size, map_entry_col_width[1], entry->perms,
			map_entry_col_width[2], entry->offset, map_entry_col_width[3], entry->dev, map_entry_col_width[4], entry->inode);

	return 0;
}

bool listProcessThreads(uint64_t pid)
{
	printf("List of threads:\n");
	printf("Not yet implemented!\n");
	printf("\n");

	return true;
}

bool listProcessMemory(uint32_t pid)
{
	bool s;
	printf("Memory Map:\n");
	printf("%-*s | %-*s | %-*s | %*s | %*s | %*s | %-*s\n",
		map_entry_col_width[0]+2, "address", map_entry_col_width[0]+2, "size", map_entry_col_width[1], "perms", map_entry_col_width[2]+2, "offset",
		map_entry_col_width[3], "dev", map_entry_col_width[4], "inode", map_entry_col_width[5], "name");
	printf("-------------------------------------------------------------------------------------------\n");

	s = parseProcessMaps(pid, &printProcMapEntry);
	printf("\n");
	return s;
}

int printProcMapEntry(ProcMapsEntry* entry, uint32_t last_module_inode, size_t line_nr)
{
	char* name = NULL;

	printf("0x%0*lx | 0x%0*lx | %*s | 0x%0*lx | %*s | %*u | ",
			map_entry_col_width[0], entry->address, map_entry_col_width[0], entry->size, map_entry_col_width[1], entry->perms,
			map_entry_col_width[2], entry->offset, map_entry_col_width[3], entry->dev, map_entry_col_width[4], entry->inode);

	if ( entry->pathname && entry->pathname[0] != 0 )
	{
		getFileNameL(entry->pathname, &name);
		printf("%s", name);
	}
	printf("\n");

	return 0;
}

bool listProcessHeaps(uint32_t pid, int type)
{
	bool s;

	printf("List of heaps:\n");
	printf("%-*s | %-*s | %-*s | %*s | %*s | %*s | %-*s\n",
		map_entry_col_width[0]+2, "address", map_entry_col_width[0]+2, "size", map_entry_col_width[1], "perms", map_entry_col_width[2]+2, "offset",
		map_entry_col_width[3], "dev", map_entry_col_width[4], "inode", map_entry_col_width[5], "name");
	printf("-------------------------------------------------------------------------------------------\n");

	s = parseProcessMaps(pid, &printProcessHeap);
	printf("\n");

	return s;
}

int printProcessHeap(ProcMapsEntry* entry, uint32_t last_module_inode, size_t line_nr)
{
	if ( !entry->pathname || strnlen(entry->pathname, PATH_MAX) == 0 )
		return 1;
	if ( strncmp(entry->pathname, "[heap]", 10) != 0 )
		return 2;

	printf("0x%0*lx | 0x%0*lx | %*s | 0x%0*lx | %*s | %*u | %s\n",
			map_entry_col_width[0], entry->address, map_entry_col_width[0], entry->size, map_entry_col_width[1], entry->perms, map_entry_col_width[2], entry->offset,
			map_entry_col_width[3], entry->dev, map_entry_col_width[4], entry->inode, entry->pathname);

	return 0;
}

/**
 * Writing is only possible as root.
 */
int writeProcessMemory(uint32_t pid, unsigned char *payload, uint32_t payload_ln, uint64_t start)
{
	char file[64];
	sprintf(file, "/proc/%u/%s", pid, "mem");
	overwrite(file, payload, payload_ln, start);

	return 0;
}

/**
 * Printing is only possible as root.
 */
//bool printProcessRegions(uint32_t pid, uint64_t start, uint8_t skip_bytes, unsigned char *needle, uint32_t needle_ln)
//{
////	int fd;
//	FILE* fp;
//	printf("printProcessRegions");
//
////	if ( !openProcessMemory(pid, &fd, O_RDONLY) )
//	if ( !fopenProcessMemory(pid, &fp, "r") )
//	{
//		printf("ERROR: Could not open fp memory %u.\n", pid);
//		return false;
//	}
//
////	ptrace(PTRACE_ATTACH, pid, 0, 0);
////	waitpid(pid, NULL, 0);
//
////	pread(fd, &buffer, sizeof(buffer), addr);
//
//	uint16_t block_size = BLOCKSIZE_LARGE;
//	uint64_t nr_of_parts = length / block_size;
//	if ( length % block_size != 0 ) nr_of_parts++;
//
//	unsigned char* block = NULL;
//	block = (unsigned char*) malloc(block_size);
//	if ( !block )
//	{
//		printf("Malloc block failed.\n");
//		return false;
//	}
//	printBlock(nr_of_parts, block, fp, block_size, start);
//
////	ptrace(PTRACE_DETACH, pid, 0, 0);
//
////	close(fd);
//	fclose(fp);
//	free(block);
//
//	return true;
//}

/**
 *
 * TODO: check out "process_vm_readv", "process_vm_writev"
 *
 * @param pid
 * @param start uint64_t absolute start offset
 * @param skip_bytes
 * @param needle
 * @param needle_ln
 * @return
 */
bool printProcessRegions(uint32_t pid, uint64_t start, uint8_t skip_bytes, unsigned char* needle, uint32_t needle_ln)
{
	FILE* fp;
	ProcMapsEntry entry;
	bool s;
	int print_s = 0;

	uint64_t base_off = 0;
	uint64_t found = FIND_FAILURE;

	char* file_name = NULL;

	uint32_t last_inode = 0;
	uint64_t last_base = 0;
	uint64_t printed_module_base = 0;

	p_needle = needle;
	p_needle_ln = needle_ln;

	// check if /proc/pid/mem is accessible
	if ( !fopenProcessMemory(pid, &fp, "r") )
	{
		printf("ERROR: Could not open process %u memory.\nRoot permissions are required!", pid);
		return false;
	}

	if ( !fopenProcessMaps(pid, &fp) )
	{
		printf("ERROR: Could not open process maps %u.\n", pid);
		return false;
	}

//	s = getRegion(start, fp, &entry);
//	if ( !s )
//	{
//		printf("ERROR (%d): No region hit!\n", s);
//		fclose(fp);
//		return false;
//	}

	if ( find_f )
		Finder_initFailure(p_needle, p_needle_ln);

//	getFileNameL(entry.pathname, &file_name);
//	last_base = entry.base;
//	printRegionInfo(&entry, file_name);
//	printf("region: 0x%lx 0x%lx 0x%x %s\n", entry.address, entry.base, entry.size, entry.pathname);

	while ( queryNextRegion(fp, &entry) )
	{
		if ( entry.inode != 0 && entry.inode == last_inode )
			entry.base = last_base;
		else
			last_base = entry.base;

		last_inode = entry.inode;

		if ( !isReadableRegion(&entry) )
			continue;

		// print_s == 1 => quit printing actual module
		// printed_module_base == entry.base => still in actual module
		if ( print_s == 1 && printed_module_base == entry.base )
		{
			continue;
		}

		if ( start > entry.address )
			continue;
		else
		{
			if ( start > 0 )
			{
				base_off = start - entry.address;
				printed_module_base = entry.base;
				print_s = 1;
			}
			start = 0;
		}

		file_name = NULL;
		getFileNameL(entry.pathname, &file_name);

		if ( printed_module_base != entry.base )
		{
			if ( !confirmContinueWithNextRegion(file_name) )
				break;
			else
				print_s = 1;
		}

		if ( print_s == 1 )
			printRegionInfo(&entry, file_name);

		/*if ( find_f )
		{
//			printf("Region Start Find\n");
			found = findNeedleInProcessMemoryBlock(entry.address, entry.RegionSize, base_off, fp, p_needle, p_needle_ln);
			if ( found == FIND_FAILURE )
			{
//				printf(" - Region Start Find: not found\n");

				if ( !getNextPrintableRegion(fp, &entry, &p, &file_name, print_s, last_base) )
					break;
				last_base = entry.address; // module base
				continue;
			}
			else
			{
				Printer_setHiglightBytes(p_needle_ln);
				found = found - (uint64_t) entry.address;
				base_off = normalizeOffset(found, &skip_bytes);
				Printer_setHiglightWait(skip_bytes);
			}
		}*/

//		printf("region: 0x%lx 0x%lx 0x%x %s\n", entry.address, entry.base, entry.size, entry.pathname);
//		Printer_setSkipBytes(skip_bytes);
		print_s = printRegionProcessMemory(pid, entry.address, base_off, entry.size, found, print_s);

//		if ( !getNextPrintableRegion(fp, &entry, &file_name, print_s, last_base) )
//			break;

//		if ( print_s == 1 )
//			printRegionInfo(&entry, file_name);
		printed_module_base = entry.base;
		base_off = 0;
	}

	fclose(fp);
	Finder_cleanUp();

	return true;
}

// find first region hitting start
//bool getRegion(uint64_t start, FILE* fp, ProcMapsEntry* entry)
//{
//	char line[LINE_BUFFER_SPACE];
//	while ( fgets(line, line_size, fp) )
//	{
//		line[line_size] = 0;
//
//		if ( !parseProcMapsLine(line, entry) )
//			return -1;
//
//		if ( addressIsInRegionRange(start, entry->address, entry->size) )
//			return 1;
//	}
//	return 0;
//}

void printRegionInfo(ProcMapsEntry* entry, const char* file_name)
{
	printf("%s (0x%lx - 0x%lx):\n", (file_name!=NULL)?file_name:"", entry->address, entry->address + entry->size);
}


///**
// * Find needle in a process memory block.
// *
// * @param me32
// * @param process
// * @param needle
// * @param needle_ln
// * @param offset
// * @return uint64_t absolute found address
// */
//uint64_t
//findNeedleInProcessMemoryBlock(BYTE* base_addr, DWORD base_size, uint64_t offset, FILE* fp, const unsigned char* needle, uint32_t needle_ln)
//{
//	uint64_t found = FIND_FAILURE;
//	uint64_t block_i;
//	uint64_t j = 0;
//	uint64_t base_off = offset;
//	size_t n_size = length;
//	unsigned char buf[BLOCKSIZE_LARGE] = {0};
//
//	debug_info("Find: ");
//	for ( block_i = 0; block_i < needle_ln; block_i++ )
//		debug_info("%02x", p_needle[block_i]);
//	debug_info("\n");
//
//	while ( n_size && n_size == length )
//	{
//		n_size = readProcessBlock(base_addr, base_size, base_off, fp, buf);
//		block_i = findNeedleInBlock(needle, needle_ln, buf, &j, n_size);
//
//		if ( j == needle_ln )
//		{
//			found = (uint64_t) base_addr + base_off + block_i - needle_ln + 1;
//			break;
//		}
//
//		base_off += block_i;
//	}
//
//	return found;
//}

//bool getNextPrintableRegion(FILE* fp, ProcMapsEntry* entry, char** file_name, int print_s, uint64_t last_base)
//{
//	bool s;
//
//	if ( print_s == 0 )
//		s = queryNextAccessibleRegion(fp, entry, file_name);
//	else
//		s = queryNextAccessibleBaseRegion(fp, entry, file_name);
//
//	if ( !s )
//		return false;
//
////	printf("last_base: %p\n", last_base);
////	printf("entry.AllocationBase: %p\n", entry->AllocationBase);
//
//	getFileNameL(entry->pathname, file_name);
////	printf("file_name: %s\n", *file_name);
//	if ( last_base != entry->base )
//	{
//		if ( !confirmContinueWithNextRegion(*file_name) )
//			return false;
//	}
//	return true;
//}

//bool queryNextAccessibleRegion(FILE* fp, ProcMapsEntry* entry, char** file_name)
//{
//	bool s;
//	s = queryNextRegion(fp, entry);
//
//	while ( s )
//	{
//		if ( !isReadableRegion(entry) )
//			s = queryNextRegion(fp, entry);
//		else
//			break;
//	}
//	return s;
//}

//bool queryNextAccessibleBaseRegion(FILE* fp, ProcMapsEntry* entry, char** file_name)
//{
//	bool s;
//	uint64_t old_base;
//
//	old_base = entry->base;
//	s = queryNextRegion(fp, entry);
//
//	while ( s )
//	{
//		if ( old_base == entry->base || !isReadableRegion(entry) )
//			s = queryNextRegion(fp, entry);
//		else
//			break;
//	}
//	return s;
//}

bool queryNextRegion(FILE* fp, ProcMapsEntry* entry)
{
	char line[LINE_BUFFER_SPACE];

	if ( !fgets(line, line_size, fp) )
		return false;

	line[line_size] = 0;

	if ( !parseProcMapsLine(line, entry) )
	{
		printf("ERROR: could not parse entry!\n");
		return false;
	}

	return true;
}

/**
 *
 * @param fp
 * @param me32
 * @param base_off uint32_t base offset in module to start at printing
 * @return 0 if end of block is reached, 1 if forced to quit
 */
int printRegionProcessMemory(uint32_t pid, uint64_t base_addr, uint64_t base_off, uint64_t size, uint64_t found, int print_s)
{
	FILE* fp;
	unsigned char block[BLOCKSIZE_LARGE] = {0};
	char input;
	uint8_t skip_bytes;
	int s = 0;
	uint16_t block_size = BLOCKSIZE_LARGE;
	uint64_t nr_of_parts = length / block_size;
	if ( length % block_size != 0 ) nr_of_parts++;
	uint64_t base_end = base_addr + size;

	if ( !fopenProcessMemory(pid, &fp, "r") )
	{
		printf("ERROR: Could not open process %u memory.\n", pid);
		return false;
	}
//	printf(" - base_addr: 0x%lx\n", base_addr);
//	printf(" - base_off: 0x%lx\n", base_off);
	base_off += base_addr;
//	printf(" - base_off: 0x%lx\n", base_off);

	// prevent auto print, if next region of a module is accessed, two prevent printing two blocks at once
	if ( print_s == 1 )
		base_off = printMemoryBlock(nr_of_parts, block, fp, block_size, base_off, base_end);
//	printf(" - base_off: 0x%lx\n", base_off);

	if ( !continuous_f )
		return 1;

	while ( base_off != UINT64_MAX )
	{
		input = getch();

		if ( input == ENTER )
		{
			base_off = printMemoryBlock(nr_of_parts, block, fp, block_size, base_off, base_end);
//			printf(" -- base_off: 0x%lx\n", base_off);
		}
//		else if ( find_f && input == 'n' )
//		{
//			found = findNeedleInProcessMemoryBlock(base_addr, size, found + p_needle_ln, fp, p_needle, p_needle_ln);
//			if ( found == FIND_FAILURE )
//			{
//				s = 0;
//				break;
//			}
//			found -= (uint64_t) base_addr;
//			printf("\n");
//			Printer_setHiglightBytes(p_needle_ln);
//			base_off = normalizeOffset(found, &skip_bytes);
//			Printer_setHiglightWait(skip_bytes);
//
//			base_addr = printMemoryBlock(nr_of_parts, block, fp, block_size, base_addr, size);
//		}
		else if ( input == 'q' )
		{
			s = 1;
			break;
		}
	}

	return s;
}

uint64_t printMemoryBlock(uint64_t nr_of_parts, unsigned char* block, FILE* fi, uint16_t block_size, uint64_t block_start, uint64_t block_max)
{
	uint64_t p;
	uint64_t read_size = 0;
	uint64_t size;
	uint64_t end = block_start + length;
	uint8_t offset_width = countHexWidth64(end);
//	printf(" - printMemoryBlock(0x%lx, %p, %p, %u, 0x%lx, 0x%lx)\n", nr_of_parts, block, fi, block_size, block_start, block_max);
//	printf(" - - end: 0x%x\n", end);
//	printf(" - - offset_width: %u\n", offset_width);

	for ( p = 0; p < nr_of_parts; p++ )
	{
		debug_info("%lu / %lu\n", (p+1), nr_of_parts);
		read_size = block_size;
		if ( block_start + read_size > end ) read_size = end - block_start;
//		printf(" - - - read_size: 0x%lx\n", read_size);

		memset(block, 0, block_size);
		size = readFile(fi, block_start, read_size, block);
//		printf(" - - - size: 0x%lx\n", size);

		if ( !size )
		{
			printf("ERROR: Reading block of bytes failed!\n");
			block_start = block_max;
			break;
		}

		printLine(block, block_start, size, offset_width);

		block_start += read_size;
//		printf(" - - - block_start: 0x%lx\n", block_start);
	}

//	printf(" - - block_start: 0x%lx\n", block_start);
	if ( block_start >= block_max )
		block_start = UINT64_MAX;

//	printf(" - - block_start: 0x%lx\n", block_start);
	return block_start;
}
