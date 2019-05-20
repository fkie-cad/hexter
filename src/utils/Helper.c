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

/**
 *
 * @param buf char[128]
 * @param prefix
 * @return
 */
int getTempFile(char* buf, char* prefix)
{
	int s = 1;
#if defined(__linux__) || defined(__linux) || defined(linux)
	snprintf(buf, 128, "/tmp/%sXXXXXX.tmp", prefix);
	buf[127] = 0;

	s = mkstemps(buf, 4);
#endif
	return s;
}
