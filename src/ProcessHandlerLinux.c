#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fnmatch.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

#include "ProcessHandlerLinux.h"
#include "utils/Converter.h"
#include "utils/Strings.h"
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

/**
 * Structure to hold parsed /proc/pid/stat values.
 * Only the needed ones are defined here.
 */
typedef struct ProcStat
{
//	uint32_t pid; // The process ID.
//	char comm[16];  // The filename of the executable, in parentheses. This is visible whether or not the executable is swapped out.
	char state; // process state: R  Running, S  Sleeping in an interruptable wait, D  Waiting in uninterruptable disk sleep, Z  Zombie, T  Stopped, t  Tracing stop , W  Paging, X  Dead, x  Dead, K  Wakekill, W  Waking, P  Parked
	uint32_t ppid; // The PID of the parent of this process.
//	uint32_t pgrp; // The process group ID of the process.
//	int session; // %d The session ID of the process.
//	int tty_nr; // %d The controlling terminal of the process.  (The minor device number is contained in the combination of bits 31 to 20 and 7 to 0; the major device number is in bits 15 to 8.)
//	int tpgid; // %d The ID of the foreground process group of the controlling terminal of the process.
	uint16_t flags; // %u The kernel flags word of the process.  For bit meanings, see the PF_* defines in the Linux kernel source file include/linux/sched.h. Details depend on the kernel version.The format for this field was %lu before Linux 2.6.
//	uint32_t minflt; // %lu The number of minor faults the process has made which have not required loading a memory page from disk.
//	uint32_t cminflt; // %lu The number of minor faults that the process's waited-for children have made.
//	uint32_t majflt; // %lu The number of major faults the process has made which have required loading a memory page from disk.
//	uint32_t cmajflt; // %lu The number of major faults that the process's waited-for children have made.
//	uint32_t utime; // %lu Amount of time that this process has been scheduled in user mode, measured in clock ticks (divide by sysconf(_SC_CLK_TCK)). This includes guest time, guest_time (time spent running a virtual CPU, see below), so that applications that are not aware of the guest time field do not lose that time from their calculations.
//	uint32_t stime; // %lu Amount of time that this process has been scheduled in kernel mode, measured in clock ticks (divide by sysconf(_SC_CLK_TCK)).
//	int32_t cutime; // %ld Amount of time that this process's waited for children have been scheduled in user mode, measured in clock ticks (divide by sysconf(_SC_CLK_TCK)).  (See also times(2).)  This includes guest time, cguest_time (time spent running a virtual CPU, see below).
//	int32_t cstime; // %ld Amount of time that this process's waited for children have been scheduled in kernel mode, measured in clock ticks (divide by sysconf(_SC_CLK_TCK)).
//	int32_t priority; // %ld (Explanation for Linux 2.6) For processes running a real-time scheduling policy (policy below; see sched_setscheduler(2)), this is the negated scheduling priority, minus one; that is, a number in the range -2 to -100, corresponding to real-time priorities 1 to 99.  For processes running under a non-real-time scheduling policy, this is the raw nice value (setpriority(2)) as represented in the kernel. The kernel stores nice values as numbers in the range 0 (high) to 39 (low), corresponding to the user-visible nice range of -20 to 19. Before Linux 2.6, this was a scaled value based on the scheduler weighting given to this process.
//	int32_t nice; // %ld The nice value (see setpriorityint32_t), a value in the range 19 (low priority) to -20 (high priority).
	int32_t num_threads; // %ld Number of threads in this process (since Linux 2.6). Before kernel 2.6, this field was hard coded to 0 as a placeholder for an earlier removed field.
//	int32_t itrealvalue; // %ld The time in jiffies before the next SIGALRM is sent to the process due to an interval timer.  Since kernel 2.6.17, this field is no longer maintained, and is hard coded as 0.
//	uint64_t starttime; // %llu The time the process started after system boot. In kernels before Linux 2.6, this value was expressed in jiffies.  Since Linux 2.6, the value is expressed in clock ticks (divide by sysconf(_SC_CLK_TCK)). The format for this field was %lu before Linux 2.6.
	uint32_t vsize; // %lu Virtual memory size in bytes.
	int32_t rss; // %ld Resident Set Size: number of pages the process has in real memory.  This is just the pages which count toward text, data, or stack space.  This does not include pages which have not been demand-loaded in, or which are swapped out.
//	uint32_t rsslim; // %lu Current soft limit in bytes on the rss of the process; see the description of RLIMIT_RSS in getrlimituint32_t.
//	uint32_t startcode; // %lu  [PT] The address above which program text can run.
//	uint32_t endcode; // %lu  [PT] The address below which program text can run.
//	uint32_t startstack; // %lu  [PT]	The address of the start (i.e., bottom) of the stack.
	uint32_t kstkesp; // %lu  [PT] The current value of ESP (stack pointer), as found in the kernel stack page for the process.
	uint32_t kstkeip; // %lu  [PT] The current EIP (instruction pointer).
//	uint32_t signal; // %lu The bitmap of pending signals, displayed as a decimal number.  Obsolete, because it does not provide information on real-time signals; use /proc/[pid]/status instead.
//	uint32_t blocked; // %lu The bitmap of blocked signals, displayed as a decimal number.  Obsolete, because it does not provide information on real-time signals; use /proc/[pid]/status instead.
//	uint32_t sigignore; // %lu The bitmap of ignored signals, displayed as a decimal number.  Obsolete, because it does not provide information on real-time signals; use /proc/[pid]/status instead.
//	uint32_t sigcatch; // %lu The bitmap of caught signals, displayed as a decimal number.  Obsolete, because it does not provide information on real-time signals; use /proc/[pid]/status instead.
//	uint32_t wchan; // %lu  [PT] This is the "channel" in which the process is waiting.  It is the address of a location in the kernel where the process is sleeping.  The corresponding symbolic name can be found in /proc/[pid]/wchan.
//	uint32_t nswap; // %lu Number of pages swapped (not maintained).
//	uint32_t cnswap; // %lu Cumulative nswap for child processes (not maintained).
//	int exit_signal; // %d  (since Linux 2.1.22) Signal to be sent to parent when we die.
//	int processor; // %d  (since Linux 2.2.8) CPU number last executed on.
//	uint16_t rt_priority; // %u  (since Linux 2.5.19) Real-time scheduling priority, a number in the range 1 to 99 for processes scheduled under a real-time policy, or 0, for non-real-time processes (see sched_setscheduler(2)).
//	uint16_t policy; // %u  (since Linux 2.5.19) Scheduling policy (see sched_setscheduler(2)). Decode using the SCHED_* constants in linux/sched.h. The format for this field was %lu before Linux 2.6.22.
//	uint64_t delayacct_blkio_ticks; // %llu  (since Linux 2.6.18) Aggregated block I/O delays, measured in clock ticks (centiseconds).
//	uint32_t guest_time; // %lu Guest time of the process (time spent running a virtual CPU for a guest operating system), measured in clock ticks (divide by sysconf(_SC_CLK_TCK)).
//	int32_t cguest_time; // %ld Guest time of the process's children, measured in clock ticks (divide by sysconf(_SC_CLK_TCK)).
	uint32_t start_data; // %lu [PT] Address above which program initialized and uninitialized (BSS) data are placed.
	uint32_t end_data; // %lu [PT] Address below which program initialized and uninitialized (BSS) data are placed.
	uint32_t start_brk; // %lu [PT] Address above which program heap can be expanded with brk(2).
	uint32_t arg_start; // %lu [PT] Address above which program command-line arguments (argv) are placed.
//	uint32_t arg_end; // %lu [PT]	Address below program command-line arguments (argv)	are placed.
//	uint32_t env_start; // %lu [PT] Address above which program environment is placed.
//	uint32_t env_end; // %lu [PT]	Address below which program environment is placed.
//	int exit_code; // %d [PT]The thread's exit status in the form reported by waitpid(2).
} ProcStat;

typedef int (*MapsInfoCallback)(ProcMapsEntry* entry, uint32_t last_module_inode, size_t line_nr);

static Bool fopenProcessFile(uint32_t pid, FILE **fp, char *type, const char* mode);
static Bool fopenProcessMemory(uint32_t pid, FILE **fp, const char* mode);
static Bool fopenProcessMaps(uint32_t pid, FILE **fp);

static Bool parseProcMapsLine(char* line, ProcMapsEntry* entry);

static void printProcessMemoryTableHeader();
static int printProcMapEntry(ProcMapsEntry* entry, uint32_t last_module_inode, size_t line_nr);

static Bool parseProcessMaps(uint32_t pid, MapsInfoCallback cb);
static void printProcessModulesTableHeader(const uint8_t map_entry_col_width[6]);
static int printProcModule(ProcMapsEntry* entry, size_t module_nr);

static void printProcessHeapsTableHeader();
static int printProcessHeap(ProcMapsEntry* entry, uint32_t last_module_inode, size_t module_nr);

static Bool getProcName(uint32_t pid, char* name, size_t name_size);
static Bool getProcStat(uint32_t pid, ProcStat* proc_stat);
char* getStateString(char c);

static Bool isModule(ProcMapsEntry* entry);
static Bool isReadableRegion(ProcMapsEntry *entry);

static Bool addressIsInRegionRange(uint64_t address, uint64_t base, uint64_t size);
static void setModuleEndAddress(ProcMapsEntry *entry, FILE *fp);
static uint64_t getModuleEndAddress(ProcMapsEntry *module, FILE* fp);
//static Bool keepLengthInModule(ProcMapsEntry *entry, uint64_t start, uint64_t *length);

static void printRegionInfo(ProcMapsEntry* entry, const char* file_name);
static Bool skipQuittedModuleRegions(ProcMapsEntry* entry, int print_s, uint64_t printed_module_base);
static Bool reachedNextModule(uint64_t printed_module_base, uint64_t entry_base);
static Bool queryNextRegion(FILE* fp, ProcMapsEntry* entry);
static int printRegionProcessMemory(uint32_t pid, uint64_t base_addr, uint64_t base_off, uint64_t size, uint64_t found, int print_s);
static uint64_t findNeedleInProcessMemoryBlock(uint32_t pid, uint64_t base_addr, uint64_t base_size, uint64_t offset, const unsigned char* needle, uint32_t needle_ln);

static int filter(const struct dirent *dir);
static void processdir(const struct dirent *dir);

static const uint8_t map_entry_col_width[6] = { 16, 5, 8, 5, 8, 8 };
#define LINE_BUFFER_SPACE 513
static const uint16_t line_size = 512;

static unsigned char* p_needle = NULL;
static uint32_t p_needle_ln;
static int errsv = 0;

/**
 * Open proc pid file
 *
 * @param	pid uint32_t the fp id
 * @param	fp FILE** the file descriptor
 * @param	tpye char* the proc file to open
 * @param	flag int the access flag: O_RDONLY, O_RDWR, O_WRONLY
 */
Bool fopenProcessFile(uint32_t pid, FILE **fp, char *type, const char* mode)
{
	char file[64];
	sprintf(file, "/proc/%u/%s", pid, type);
	file[63] = 0;

	errno = 0;
	*fp = fopen(file, mode);
	errsv = errno;

	if ( !(*fp) )
		return false;

	return true;
}

/**
 * -rw------- ... mem
 */
Bool fopenProcessMemory(uint32_t pid, FILE **fp, const char* mode)
{
	return fopenProcessFile(pid, fp, "mem", mode);
}

/**
 * -r--r--r-- ... maps
 */
Bool fopenProcessMaps(uint32_t pid, FILE **fp)
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
Bool parseProcessMaps(uint32_t pid, MapsInfoCallback cb)
{
	FILE* fp = NULL;
	const uint16_t line_size = 512;
	char line[513];
	size_t line_nr = 0;
	Bool last_module_inode = 0;
	ProcMapsEntry entry;

	if ( !fopenProcessMaps(pid, &fp) )
	{
		printf("ERROR (%x): Could not open process maps %u.\n", errsv, pid);
		return false;
	}
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

Bool parseProcMapsLine(char* line, ProcMapsEntry* entry)
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

uint8_t makeStartHitAccessableMemory(uint32_t pid, uint64_t *start)
{
	FILE *fp = NULL;
	char line[LINE_BUFFER_SPACE];

	uint8_t info_line_break = 0;

	ProcMapsEntry entry;
	memset(&entry, 0, sizeof(entry));

	if ( !fopenProcessMaps(pid, &fp) )
	{
		printf("ERROR (%x): Could not open process maps %u.\n", errsv, pid);
		return 0;
	}

//	while ( queryNextRegion(fp, &entry) )
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
//			info_line_break = keepLengthInModule(&entry, *start, &length);
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
	(*start) =  entry.address;

	setModuleEndAddress(&entry, fp);

	fclose(fp);

	return info_line_break;
}

Bool isReadableRegion(ProcMapsEntry *entry)
{
	return entry->perms[0] == 'r';
}

Bool addressIsInRegionRange(uint64_t address, uint64_t base, uint64_t size)
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

//Bool keepLengthInModule(ProcMapsEntry *entry, uint64_t start, uint64_t *length)
//{
//	uint64_t base_off = start - entry->address;
//	if ( base_off + *length > entry->size )
//	{
//		printf("Info: Length 0x%lx does not fit in region!\nSetting it to 0x%lx!\n", *length, entry->size - base_off);
//		*length = entry->size - base_off;
//		return true;
//	}
//	return false;
//}

/**
 * Actually gets the size of the main program process.
 */
uint64_t getSizeOfProcess(uint32_t pid)
{
	FILE *fp = NULL;
	const uint16_t line_size = 512;
	char line[513];
	char proc_name[513] = {0};

	ProcMapsEntry entry;
	memset(&entry, 0, sizeof(entry));
	ProcMapsEntry last_entry;
	memset(&entry, 0, sizeof(entry));
	uint64_t start_address = 0;

	char* module_name = NULL;
	Bool proc_module_started = false;

	if ( !getProcName(pid, proc_name, 512) )
		return 0;

	if ( !fopenProcessMaps(pid, &fp) )
	{
		printf("ERROR (%x): Could not open process maps %u.\n", errsv, pid);
		return 0;
	}
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
 * Get fp name
 *
 * @param	pid uint32_t
 * @param	name char* allocated space for the name
 * @param	name_size size_t
 */
Bool getProcName(uint32_t pid, char* name, size_t name_size)
{
	FILE *fp = NULL;
	char line[LINE_BUFFER_SPACE];
	char* tmp_name;
	char* bucket[1];
	size_t bucket_ln;

	// -r--r--r-- ... cmdline
	if ( !fopenProcessFile(pid, &fp, "cmdline", "r") )
	{
		printf("ERROR (%x): failed to open /proc/%u/cmdline\n", errsv, pid);
		return false;
	}
	fgets(line, line_size, fp);
	line[line_size] = 0;
	fclose(fp);

	bucket_ln = split(line, " ", bucket, 1);
	if ( bucket_ln == 0 )
		return false;

	getFileNameL(bucket[0], &tmp_name);

	memcpy(name, tmp_name, strnlen(tmp_name, name_size));

	return true;
}

/**
 * Get proc pid stats
 *
 * @param	pid uint32_t
 * @param	proc_stats ProcStat* structure to fill
 */
Bool getProcStat(uint32_t pid, ProcStat* proc_stat)
{
	FILE* fp = NULL;
	char line[LINE_BUFFER_SPACE];
	const uint8_t bucket_max = 53;
	char* bucket[53]; // one more entry to mark uncommon value of comm
	size_t bucket_ln;

	if ( !fopenProcessFile(pid, &fp, "stat", "r") )
	{
		printf("ERROR (%x): failed to open /proc/%u/stat\n", errsv, pid);
		return false;
	}
	fgets(line, line_size, fp);
	line[line_size] = 0;
	fclose(fp);

	bucket_ln = splitArgsCSM(line, bucket, bucket_max, '(', ')');
	if ( bucket_ln == 0 || bucket_ln >= bucket_max )
	{
//		printf("bucket_ln: %llu\n", bucket_ln);
//		int i = 0;
//		for ( i = 0; i < bucket_ln; i++ )
//			printf("%d: %s\n", i, bucket[i]);
		return false;
	}

	parseUint32(bucket[3], &proc_stat->ppid, 10);
	proc_stat->state = bucket[2][0];
	parseUint16(bucket[8], &proc_stat->flags, 10);
	parseUint32(bucket[19], &proc_stat->num_threads, 10);
	parseUint32(bucket[22], &proc_stat->vsize, 10);
	parseUint32(bucket[23], &proc_stat->rss, 10);

	return true;
}

/**
 * List all fp modules.
 *
 * @param	pid uint32_t the target fp pid
 * @return	Bool success state
 */
Bool listProcessModules(uint32_t pid)
{
	FILE *fp = NULL;
//	char line[LINE_BUFFER_SPACE];

	uint32_t last_module_inode = 0;
	uint64_t module_size = 0;
	uint64_t module_nr = 0;
	uint64_t start_address = 0;

	ProcMapsEntry entry;
	memset(&entry, 0, sizeof(entry));
	ProcMapsEntry last_entry;
	memset(&entry, 0, sizeof(entry));

	if ( !fopenProcessMaps(pid, &fp) )
	{
		printf("ERROR (%x): Could not open process maps %u.\n", errsv, pid);
		return false;
	}

	printProcessModulesTableHeader(map_entry_col_width);

	while ( queryNextRegion(fp, &entry) )
//	while ( fgets(line, line_size, fp) )
	{
//		line[line_size] = 0;

//		if ( !parseProcMapsLine(line, &entry) )
//			break;

		if ( !isModule(&entry) )
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
			printProcModule(&last_entry, module_nr);
		}

		last_module_inode = entry.inode;

//		memset(&last_entry, 0, sizeof(last_entry));
		memcpy(&last_entry, &entry, sizeof(entry));
	}
	module_nr++;
	last_entry.size = module_size;
	last_entry.address = start_address;
	printProcModule(&last_entry, module_nr);

	printf("\n");

	fclose(fp);

	return true;
}

Bool isModule(ProcMapsEntry* entry)
{
	return entry->pathname[0] != 0 && entry->pathname[0] != '[' && entry->inode != 0;
}

void printProcessModulesTableHeader(const uint8_t map_entry_col_width[6])
{
	printf("List of modules:\n");
	printf(" - %-*s | %-*s | %-*s | %*s | %*s | %*s\n",
			map_entry_col_width[0] + 2, "address", map_entry_col_width[0] + 2, "size", map_entry_col_width[1], "perms",
			map_entry_col_width[2] + 2, "offset", map_entry_col_width[3], "dev", map_entry_col_width[4], "inode");
	printf("----------------------------------------------------------------------------------\n");
}

int printProcModule(ProcMapsEntry* entry, size_t module_nr)
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

Bool listProcessThreads(uint64_t pid)
{
	printf("List of threads:\n");
	printf("Not yet implemented!\n");
	printf("\n");

	return true;
}

Bool listProcessMemory(uint32_t pid)
{
	Bool s;

	printProcessMemoryTableHeader();
	s = parseProcessMaps(pid, &printProcMapEntry);
	printf("\n");
	return s;
}

void printProcessMemoryTableHeader()
{
	printf("Memory Map:\n");
	printf("%-*s | %-*s | %-*s | %*s | %*s | %*s | %-*s\n",
		map_entry_col_width[0]+2, "address", map_entry_col_width[0]+2, "size", map_entry_col_width[1], "perms", map_entry_col_width[2]+2, "offset",
		map_entry_col_width[3], "dev", map_entry_col_width[4], "inode", map_entry_col_width[5], "name");
	printf("-------------------+--------------------+-------+------------+-------+----------+----------\n");
}

int printProcMapEntry(ProcMapsEntry* entry, uint32_t last_module_inode, size_t line_nr)
{
	char* name = NULL;

	if ( !isReadableRegion(entry) )
		setAnsiFormat(LIGHT_STYLE);

	printf("0x%0*lx | 0x%0*lx | %*s | 0x%0*lx | %*s | %*u | ",
			map_entry_col_width[0], entry->address, map_entry_col_width[0], entry->size, map_entry_col_width[1], entry->perms,
			map_entry_col_width[2], entry->offset, map_entry_col_width[3], entry->dev, map_entry_col_width[4], entry->inode);

	if ( entry->pathname[0] != 0 )
	{
		getFileNameL(entry->pathname, &name);
		printf("%s", name);
	}
	printf("\n");

	if ( !isReadableRegion(entry) )
		resetAnsiFormat();

	return 0;
}

/**
 * List process heaps
 * 
 * @param pid uint32_t
 * @param type int (Winodws only)
 */
Bool listProcessHeaps(uint32_t pid, int type)
{
	Bool s;

	printProcessHeapsTableHeader();
	s = parseProcessMaps(pid, &printProcessHeap);
	printf("\n");

	return s;
}

void printProcessHeapsTableHeader()
{
	printf("List of heaps:\n");
	printf("%-*s | %-*s | %-*s | %*s | %*s | %*s | %-*s\n",
		map_entry_col_width[0]+2, "address", map_entry_col_width[0]+2, "size", map_entry_col_width[1], "perms", map_entry_col_width[2]+2, "offset",
		map_entry_col_width[3], "dev", map_entry_col_width[4], "inode", map_entry_col_width[5], "name");
	printf("-------------------------------------------------------------------------------------------\n");
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
	file[63] = 0;
	overwrite(file, payload, payload_ln, start);

	return 0;
}

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
Bool printProcessRegions(uint32_t pid, uint64_t start, uint8_t skip_bytes, unsigned char* needle, uint32_t needle_ln)
{
	FILE* fp;
	ProcMapsEntry entry;
//	Bool s;
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
		printf("ERROR (%x): Could not open process %u memory.\nRoot permissions are required!", errsv, pid);
		return false;
	}
	fclose(fp); // maybe hold and pass pointer to mem, since it is used anyway

	if ( !fopenProcessMaps(pid, &fp) )
	{
		printf("ERROR (%x): Could not open process maps %u.\n", errsv, pid);
		return false;
	}

	if ( find_f )
		Finder_initFailure(p_needle, p_needle_ln);

	while ( queryNextRegion(fp, &entry) )
	{
//		printf("next region:\n");
		// set correct entry base
		if ( entry.inode != 0 && entry.inode == last_inode )
			entry.base = last_base;
		else
			last_base = entry.base;

		last_inode = entry.inode;

		if ( !isReadableRegion(&entry) )
			continue;

		if ( skipQuittedModuleRegions(&entry, print_s, printed_module_base) )
			continue;

		// skip until start offset is reached
		if ( start > entry.address )
			continue;
		else
		{
			// first time, start offset is reached, init variables correctly
			if ( start > 0 )
			{
				base_off = start - entry.address;
				printed_module_base = entry.base;
				print_s = 1;
			}
			// not needed anymore
			start = 0;
		}

		file_name = NULL;
		getFileNameL(entry.pathname, &file_name);

		if ( reachedNextModule(printed_module_base, entry.base) )
		{
			if ( !confirmContinueWithNextRegion(file_name, entry.address) )
				break;
			else
				print_s = 1;
		}

		if ( print_s == 1 )
			printRegionInfo(&entry, file_name);

		if ( find_f )
		{
			found = findNeedleInProcessMemoryBlock(pid, entry.address, entry.size, base_off, p_needle, p_needle_ln);
			if ( found == FIND_FAILURE )
			{
				printed_module_base = entry.base;
				base_off = 0;
				continue;
			}
			else
			{
				found = found - entry.address;
				base_off = normalizeOffset(found, &skip_bytes);
				Printer_setHighlightBytes(p_needle_ln);
				Printer_setHighlightWait(skip_bytes);
				skip_bytes = 0;
				print_s = 1;
			}
		}

//		Printer_setSkipBytes(skip_bytes);
		print_s = printRegionProcessMemory(pid, entry.address, base_off, entry.size, found, print_s);

		printed_module_base = entry.base;
		base_off = 0;
	}

	fclose(fp);
	Finder_cleanUp();

	return true;
}

Bool queryNextRegion(FILE* fp, ProcMapsEntry* entry)
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

void printRegionInfo(ProcMapsEntry* entry, const char* file_name)
{
	printf("%s (0x%lx - 0x%lx):\n", (file_name!=NULL)?file_name:"", entry->address, entry->address + entry->size);
}

/**
 * Check if region belongs to a quitted module (user pressed 'q').
 *
 * print_s == 1 => quit printing last entry (module) has been chosen
 * printed_module_base == entry.base => still in actual module
 */
Bool skipQuittedModuleRegions(ProcMapsEntry* entry, int print_s, uint64_t printed_module_base)
{
	return print_s == 1 && printed_module_base == entry->base;
}

Bool reachedNextModule(uint64_t printed_module_base, uint64_t entry_base)
{
	return printed_module_base != entry_base;
}

/**
 * Find needle in a process memory block.
 *
 * @param pid
 * @param base_addr
 * @param base_size
 * @param offset
 * @param needle
 * @param needle_ln
 * @return uint64_t absolute found address
 */
uint64_t
findNeedleInProcessMemoryBlock(uint32_t pid, uint64_t base_addr, uint64_t base_size, uint64_t offset, const unsigned char* needle, uint32_t needle_ln)
{
	FILE* fp;
	uint64_t found = FIND_FAILURE;
//	print("findNeedleInProcessMemoryBlock(0x%lx, 0x%lx, 0x%lx)\n", base_addr, base_size, offset);

	if ( !fopenProcessMemory(pid, &fp, "r") )
	{
		printf("ERROR (%x): Could not open process %u memory.\n", errsv, pid);
		return false;
	}
	found = findNeedleInFP(needle, needle_ln, base_addr+offset, fp, base_addr+base_size);

	fclose(fp);
	return found;
}

/**
 *
 * @param pid uint32_t process id
 * @param base_addr uint64_t module base address
 * @param base_off uint64_t base offset in module to start at printing
 * @param size uint64_t size of module
 * @param found uint64_t found offset
 * @param print_s int auto print flag
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

	// older linux ??
//	ptrace(PTRACE_ATTACH, pid, 0, 0);
//	waitpid(pid, NULL, 0);
//
//	off_t addr = ...; // target process address
//	pread(fd, &value, sizeof(value), addr);
//	// or
//	pwrite(fd, &value, sizeof(value), addr);
//
//	ptrace(PTRACE_DETACH, pid, 0, 0);


	if ( !fopenProcessMemory(pid, &fp, "r") )
	{
		printf("ERROR (%x): Could not open process %u memory.\n", errsv, pid);
		return false;
	}
//	printf("printRegionProcessMemory\n");
//	printf(" - base_addr: 0x%lx\n", base_addr);
//	printf(" - base_off: 0x%lx\n", base_off);
//	printf(" - size: 0x%lx\n", size);
//	printf(" - found: 0x%lx\n", found);
	base_off += base_addr;
//	printf(" - base_off: 0x%lx\n", base_off);

	// prevent auto print, if next region of a module is accessed, to prevent printing two blocks at once
	if ( print_s == 1 )
		base_off = printBlock(nr_of_parts, block, fp, block_size, base_off, base_end);
//	printf(" - base_off: 0x%lx\n", base_off);

	if ( !continuous_f )
	{
		fclose(fp);
		return 1;
	}
	while ( base_off != UINT64_MAX )
	{
		input = getch();

		if ( input == ENTER )
		{
			base_off = printBlock(nr_of_parts, block, fp, block_size, base_off, base_end);
//			printf(" -- base_off: 0x%lx\n", base_off);
		}
		else if ( find_f && input == NEXT )
		{
			found = findNeedleInProcessMemoryBlock(pid, base_addr, size, found + p_needle_ln, p_needle, p_needle_ln);
			if ( found == FIND_FAILURE )
			{
				s = 0;
				break;
			}

			found -= base_addr;
			base_off = normalizeOffset(found, &skip_bytes);
			Printer_setHighlightBytes(p_needle_ln);
			Printer_setHighlightWait(skip_bytes);
			skip_bytes = 0;

			printf("\n");
			base_off = printBlock(nr_of_parts, block, fp, block_size, base_addr+base_off, base_end);
		}
		else if ( input == QUIT )
		{
			s = 1;
			break;
		}
	}
	fclose(fp);

	return s;
}

Bool listRunningProcesses()
{
	struct dirent **namelist;
	int n;

	printf("List of processes\n");
	printf("%-10s | %-10s | %8s | %10s | %10s |  %7s | %s\n", "pid", "ppid", "threads", "vsize", "rss", "state", "name");
	printf("-----------+------------+----------+------------+------------+----------+---------------\n");

	n = scandir("/proc", &namelist, filter, 0);
	if ( n < 0 )
		perror("listRunningProcesses: scandir failed.");
	else
	{
		while ( n-- )
		{
			processdir(namelist[n]);
			free(namelist[n]);
		}
		free(namelist);
	}

	printf("\n");

	return 0;
}

int filter(const struct dirent *dir)
{
	uid_t user;
	struct stat dirinfo;
	size_t len = strlen(dir->d_name) + 7;
	char path[len];

	strcpy(path, "/proc/");
	strcat(path, dir->d_name);
	user = getuid();
	if ( stat(path, &dirinfo) < 0 )
	{
		perror("processdir() ==> stat()");
		exit(EXIT_FAILURE);
	}
//	return !fnmatch("[1-9]*", dir->d_name, 0);
	return !fnmatch("[1-9]*", dir->d_name, 0) && user == dirinfo.st_uid;
}

void processdir(const struct dirent *dir)
{
	uint32_t pid;
	size_t len = strlen(dir->d_name) + 7;
	char path[len];
	char proc_name[513] = {0};
	ProcStat proc_stat;

	strcpy(path, "/proc/");
	strcat(path, dir->d_name);

//	listFilesOfDir(path);

	parseUint32(dir->d_name, &pid, 10);
	getProcName(pid, proc_name, 512);
	if ( !getProcStat(pid, &proc_stat) )
		printf("Failed to parse /proc/pid/stat");

	printf("0x%08x | 0x%08x | %8lu | 0x%08x | 0x%08x | %8s | %s\n",
			pid, proc_stat.ppid, proc_stat.num_threads, proc_stat.vsize, proc_stat.rss, getStateString(proc_stat.state), proc_name);
}

char* getStateString(char c)
{
	switch ( c )
	{
		case 'R': return "Running";
		case 'S': return "Sleeping";
		case 'D': return "Waiting";
		case 'Z': return "Zombie";
		case 'T': return "Stopped";
		case 't': return "Tracing";
		case 'X': // return "Dead";
		case 'x': return "Dead";
		case 'K': return "Wakekill";
//		case 'W': return "Paging"; // before Linux 2.6.0
		case 'W': return "Waking";
		case 'P': return "Parked";
		default:
			return "None";
	}
}
