#include <stdio.h>
#include <errno.h>

#include "common_fileio.h"

static int errsv;

// Get file size.
// Returns actual size in bytes.
size_t getSize(const char* finame)
{
	// Read in file
	FILE* fi;
	size_t pos = 0, Filesize = 0;
	errno = 0;
	fi = fopen(finame, "rb");
	errsv = errno;

	if ( !fi )
	{
		printf("ERROR (0x%x): Could not open \"%s\".\n", errsv, finame);
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

size_t readFile(FILE* fi, size_t begin, size_t size, unsigned char* data)
{
	size_t n = 0;

	fseek(fi, begin, SEEK_SET);
	errno = 0;
	n = fread(data, 1, size, fi);
	errsv = errno;

	return n;
}

int cfio_getErrno()
{
	return errsv;
}
