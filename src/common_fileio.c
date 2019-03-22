#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "common_fileio.h"

// Get file size.
// Returns actual size in bytes.
uint64_t getSize(const char* finame)
{
    // Read in file
    FILE * fi;
    uint64_t pos=0,Filesize=0;
    fi = fopen (finame, "rb" );

    if (!fi)
	{
		printf("File %s does not exist.\n",finame);
		return 0;
	}

    pos = ftell(fi);
    fseek(fi,0,SEEK_END);
    Filesize = ftell(fi);
    fseek(fi,pos,SEEK_SET);
    fclose(fi);

    // printf("Filesize: 0x%x (dez. %d)\n",Filesize,Filesize);

    return Filesize;
}

// Uses MALLOC.
// Caller is responsible for freeing this!
uint64_t readCharArrayFile(const char* finame, unsigned char ** pData, uint64_t begin, uint64_t stopAt)
{
    FILE * fi;
    unsigned char * data = NULL;
    uint64_t Filesize=0, n=0;
    
    Filesize = getSize(finame);

    // Check Filesize == 0.
    if (!Filesize) 
    {
        printf("File %s is a null (0 bytes) file.\n",finame);
        return 0;
    }
    
    if (begin >= Filesize) 
    {
        printf("Start offset '0x%x' is beyond filesize 0x%x!\n", begin,Filesize);
        return 0;
    }
    if (stopAt > Filesize)
    {
        printf("End offset '0x%x' is beyond filesize 0x%x!\n", begin,Filesize);
        return 0;
    }
    
    // 'begin' defaults to zero and 'stopAt' defaults to Filesize.
    
    if (stopAt)
    {
        if (begin)
        {
            // Allright
            if (begin < stopAt) Filesize = stopAt - begin;
            
            // User provided us with nonsense. Use something sane instead.
            else Filesize = stopAt;
        }
        else Filesize = stopAt;  // Allright as well
    }
            
    if ((begin) && (!(stopAt)))
    {
        Filesize -= begin;
    }

    // Check Filesize == 0.
    if (!Filesize) 
    {
        printf("Filesize is 0 after using offset begin: 0x%x and stop: 0x%x.\n",begin,stopAt);
        return 0;
    }

    // Allocate space
    data = (unsigned char *) malloc(Filesize);
    if (!data) 
    {
        printf("Malloc failed.\n");
        return 0;
    }

    memset(data,0,Filesize);

    // Read I/O
    fi = fopen (finame, "rb" );
    if (!fi)
	{
		printf("File %s does not exist.\n",finame);
		return 0;
	}
        
    if (begin)
    {
        fseek(fi,begin,SEEK_SET);
    }

    n = fread(data,1,Filesize,fi);
    fclose(fi);

    *pData = data;

    return n; 
    // returns read data points (char's in this case)
}
