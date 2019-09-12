#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "common_fileio.h"

// Get file size.
// Returns actual size in bytes.
size_t getSize(const char* finame)
{
	// Read in file
	FILE* fi;
	size_t pos = 0, Filesize = 0;
	fi = fopen(finame, "rb");

	if ( !fi )
	{
		printf("File %s does not exist.\n", finame);
		return 0;
	}

	pos = ftell(fi);
	fseek(fi, 0, SEEK_END);
	Filesize = ftell(fi);
	fseek(fi, pos, SEEK_SET);
	fclose(fi);

	// printf("Filesize: 0x%x (dez. %d)\n",Filesize,Filesize);

	return Filesize;
}

//// Uses MALLOC.
//// Caller is responsible for freeing this!
//size_t readCharArrayFile(const char* finame, unsigned char** pData, size_t begin, size_t stopAt)
//{
//	unsigned char* data = NULL;
//	size_t file_size = 0, n = 0;
//
//	file_size = getSize(finame);
//	file_size = cfio_sanitizeFileSize(finame, file_size, begin, stopAt);
//
//	// Check file_size == 0.
//	if ( !file_size )
//	{
//		printf("file_size is 0 after using offset begin: 0x%lx and stop: 0x%lx.\n", begin, stopAt);
//		return 0;
//	}
//
//	// Allocate space
//	data = (unsigned char*) malloc(file_size);
//	if ( !data )
//	{
//		printf("Malloc failed.\n");
//		return 0;
//	}
//
//	memset(data, 0, file_size);
//
//	// Read I/O
//	n = cfio_readFile(finame, begin, file_size, data);
//
//	*pData = data;
//
//	return n;
//	// returns read data points (char's in this case)
//}

//// Uses malloced data to fill.
//size_t readCharArrayFileNA(const char* finame, unsigned char* data, size_t data_size, size_t begin, size_t stopAt)
//{
//	size_t file_size = 0, n = 0;
//
//	file_size = getSize(finame);
//	file_size = cfio_sanitizeFileSize(finame, file_size, begin, stopAt);
//
//	// Check file_size == 0.
//	if ( !file_size )
//	{
//		printf("Filesize is 0 after using offset begin: 0x%lx and stop: 0x%lx.\n", begin, stopAt);
//		return 0;
//	}
//	if ( file_size > data_size )
//	{
//		printf("Filesize is > data_size: 0x%lx > 0x%lx.\n", file_size, data_size);
//		return 0;
//	}
//	memset(data, 0, data_size);
//
//	// Read I/O
//	n = cfio_readFile(finame, begin, file_size, data);
//
//	return n;
//	// returns read data points (char's in this case)
//}

//size_t cfio_sanitizeFileSize(const char* finame, size_t file_size, size_t begin, size_t stopAt)
//{
//	// Check file_size == 0.
//	if ( !file_size )
//	{
//		printf("File %s is a null (0 bytes) file.\n", finame);
//		return 0;
//	}
//
//	if ( begin >= file_size )
//	{
//		printf("Start offset '0x%lx' is beyond filesize 0x%lx!\n", begin, file_size);
//		return 0;
//	}
//	if ( stopAt > file_size )
//	{
//		printf("End offset '0x%lx' is beyond filesize 0x%lx!\n", begin, file_size);
//		return 0;
//	}
//
//	// 'begin' defaults to zero and 'stopAt' defaults to file_size.
//
//	if ( stopAt )
//	{
//		if ( begin )
//		{
//			// Allright
//			if ( begin < stopAt ) file_size = stopAt - begin;
//
//				// User provided us with nonsense. Use something sane instead.
//			else file_size = stopAt;
//		}
//		else file_size = stopAt;  // Allright as well
//	}
//
//	if ((begin) && (!(stopAt)))
//	{
//		file_size -= begin;
//	}
//
//	return file_size;
//}

//size_t cfio_readFile(const char* finame, size_t begin, size_t size, unsigned char* data)
//{
//	FILE* fi;
//	size_t n = 0;
//
//	fi = fopen(finame, "rb");
//	if ( !fi )
//	{
//		printf("File %s does not exist.\n", finame);
//		return 0;
//	}
//
//	fseek(fi, begin, SEEK_SET);
//	n = fread(data, 1, size, fi);
//
//	fclose(fi);
//
//	return n;
//}

size_t readFile(FILE* fi, size_t begin, size_t size, unsigned char* data)
{
	size_t n = 0;

	fseek(fi, begin, SEEK_SET);
	n = fread(data, 1, size, fi);

	return n;
}
