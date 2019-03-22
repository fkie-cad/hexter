#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "Helper.h"
#include "../Globals.h"

void expandFilePath(char* src, char* dest)
{
	const char* env_home;
	if ( src[0] == '~' )
	{
		env_home = getenv("HOME");
		if ( env_home != NULL )
		{
			snprintf(dest, PATH_MAX, "%s/%s", env_home, &src[2]);
		}
		else
		{
			snprintf(dest, PATH_MAX, "%s", src);
		}
	}
	else
	{
		snprintf(dest, PATH_MAX, "%s", src);
	}
	dest[PATH_MAX-1] = 0;
}
