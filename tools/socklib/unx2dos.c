#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#ifdef __PUREC__
#include <tos.h>
#else
#include <mint/osbind.h>
#endif
#include <sys/types.h>
#include <sys/param.h>
#include "stsocket.h"

#ifndef ENAMETOOLONG
# define ENAMETOOLONG 86
#endif


static int drive_u_exists(void)
{
	static int exists = -1;
	
	if (exists < 0)
	{
		exists = (Dgetdrv() & (1L << (UNIDRV))) != 0;
	}
	return exists;
}


/*
 * returns 0 for ordinary files, 1 for special files (like /dev/tty)
 *
 * or returns -1 on error, such as ENAMETOOLONG
 */

int _unx2dos(const char *unx, char *dos, size_t len)
{
	int unx_length = (int)strlen(unx);
	int count = 0;
	const char *u;
	char *d;
	char c;

	dos[0] = 0;
	len--;								/* for terminating NUL */
	u = unx;
	d = dos;
	if (strncmp(u, "/dev/", 5) == 0)
	{
		u += 5;
		/* make /dev/A/foo the same as A:/foo */

		if (*u && isalpha(*u) && (u[1] == 0 || (u[1] == '/' || u[1] == '\\')))
		{
			d[0] = *u++;
			d[1] = ':';
			d += 2;
			len -= 2;
		}
		/* check for a unix device name */
		else if (drive_u_exists())
		{
			strcpy(d, "U:\\dev\\");
			d += 7;
			len -= 7;
		} else
		{
			strncpy(d, u, len);
			len -= strlen(u);
			strncat(d, ":", len);
			if (!strcmp(d, "tty:"))
				strcpy(d, "con:");
			return 1;
		}
	} else if (drive_u_exists() && strncmp(u, "/pipe/", 6) == 0)
	{
		u += 6;
		strcpy(d, "U:\\pipe\\");
		d += 8;
		len -= 8;
	} else if (*u == '/')
	{
		*d++ = drive_u_exists() ? 'U' : 'C';
		*d++ = ':';
		len -= 2;
	}

	while ((c = *u++) != 0)
	{
		count++;
		if (c == '/')
			c = '\\';
		*d++ = c;
		len--;
		if (len == 0)
		{
			if (count < unx_length)
			{
				__set_errno(ENAMETOOLONG);
				*d = 0;
				return -1;
			}
			break;
		}
	}
	*d = 0;
	return 0;
}


#if 0
int _dos2unx(const char *dos, char *unx, size_t len)
{
	int dos_length = (int)strlen(dos);
	int count = 0;
	char c;

	len--;								/* for terminating NUL */
	/* replace A:\x with /dev/a/x,
	 * replace A:\x with /x, if _rootdir is 'a',
	 * replace A:\x with /a/x, if _rootdir is 'u'.
	 * BUG/FEATURE: A:x is converted to A:\x, you lose the feature
	 *              of one current directory per device.
	 *              This is because we assume that /dev/a/x is always 
	 *              an absolute path.
	 */
	if (*dos && dos[1] == ':')
	{
		char dev = tolower(*dos);

		dos += 2;
		if (dev != _rootdir)
		{
			if (_rootdir != 'u')
			{
				*unx++ = '/';
				*unx++ = 'd';
				*unx++ = 'e';
				*unx++ = 'v';
				len -= 4;
			}
			*unx++ = '/';
			*unx++ = dev;
			len -= 2;
		}
		if (*dos != '/' && *dos != '\\')
		{
			*unx++ = '/';
			len--;
		}
	}
	/* convert slashes
	 */
	while ((c = *dos++) != 0)
	{
		count++;
		if (c == '\\')
			c = '/';
		*unx++ = c;
		len--;
		if (len == 0)
		{
			if (count < dos_length)
			{
				__set_errno(ENAMETOOLONG);
				*unx = 0;
				return -1;
			}
			break;
		}
	}
	*unx = 0;
	return 0;
}
#endif
