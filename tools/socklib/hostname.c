/* gethostname -- for now, fake by looking in environment */
/* (written by Eric R. Smith, placed in the public domain) */

/* Modified to support Pure-C, Thorsten Otto. */

#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#ifdef __GNUC__
#include <unistd.h>
#endif
#include <errno.h>
#ifdef __PUREC__
#include <tos.h>
#else
#include <mint/mintbind.h>
#include <mint/sysctl.h>
#endif

#ifndef CTL_KERN
# define CTL_KERN	1		/* "high kernel" */
# define KERN_HOSTNAME		 5	/* string: hostname */
#endif

#ifndef ENAMETOOLONG
# define ENAMETOOLONG 86
#endif

/* Changed by Guido Flohr:  Include sys/param.h for maximum hostname
   length.  */

#include <sys/param.h>
#include <sys/types.h>

#undef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 127


int gethostname(char *buf, size_t len)
{
	if (!buf)
	{
		__set_errno(EINVAL);
		return -1;
	}

	/* try first the new syscall */
	{
		long mib[2];
		size_t size;

		mib[0] = CTL_KERN;
		mib[1] = KERN_HOSTNAME;
		size = len;

		if (Psysctl(mib, 2, buf, &size, NULL, 0) == 0)
		{
			if (strcmp(buf, "(none)"))
				return 0;
		}
	}

	/* fall back to the old method */
	{
		const char *foo = 0;
		char xbuf[MAXHOSTNAMELEN + 1];
		int fd, r;
		size_t real_length;

		/* try looking for the file /etc/hostname; if it's present,
		 * it contains the name, otherwise we try the environment
		 */
		fd = (int)Fopen("U:\\etc\\hostname", O_RDONLY);
		if (fd >= 0)
		{
			r = (int)Fread(fd, MAXHOSTNAMELEN, xbuf);
			if (r > 0)
			{
				char *p;

				xbuf[r] = 0;
				p = xbuf;
				while (*p)
				{
					if (*p == '\r' || *p == '\n')
					{
						*p = 0;
						break;
					}
					++p;
				}
				foo = xbuf;
			}
			Fclose(fd);
		}

		if (foo == NULL)
			foo = getenv("HOSTNAME");
		if (foo == NULL)
			foo = "unknown";

		/* Changed by Guido Flohr: Warn if buffer was too small.  */
		real_length = strlen(foo);

#if __GNUC_PREREQ(8, 0)
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif

		strncpy(buf, foo, len < MAXHOSTNAMELEN ? len : MAXHOSTNAMELEN);

		if (real_length > len)
		{
			__set_errno(ENAMETOOLONG);
			return -1;
		}

		return 0;
	}
}
