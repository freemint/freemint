/*
 * connect() emulation for MiNT-Net, (w) '93, kay roemer.
 *
 * Modified to support Pure-C, Thorsten Otto.
 */

#include "stsocket.h"
#include "mintsock.h"

int connect(int fd, const struct sockaddr *addr, socklen_t addrlen)
{
	int r;

#if !defined(MAGIC_ONLY)
	if (__libc_newsockets)
	{
		r = (int)Fconnect(fd, addr, addrlen);

		if (r != -ENOSYS)
		{
			if (r < 0)
			{
				__set_errno(-(int)r);
				return -1;
			}
			return 0;
		}
		__libc_newsockets = 0;
	}
#endif

	{
		struct connect_cmd cmd;

#ifdef UNIX_DOMAIN_SUPPORT
		if (addr && addr->sa_family == AF_UNIX)
		{
			struct sockaddr_un un;
			struct sockaddr_un *unp = (struct sockaddr_un *) addr;

			if (addrlen <= UN_OFFSET || addrlen > sizeof(un))
			{
				__set_errno(EINVAL);
				return -1;
			}

			un.sun_family = AF_UNIX;
			_unx2dos(unp->sun_path, un.sun_path, PATH_MAX);
			un.sun_path[sizeof(un.sun_path) - 1] = '\0';

			cmd.addr = (struct sockaddr *) &un;
			cmd.addrlen = UN_OFFSET + (short) strlen(un.sun_path);
		} else
#endif
		{
			cmd.addr = (struct sockaddr *)addr;
			cmd.addrlen = (short) addrlen;
		}
		cmd.cmd = CONNECT_CMD;

		r = (int)Fcntl(fd, (long) &cmd, SOCKETCALL);
		if (r < 0)
		{
			__set_errno(-(int)r);
			return -1;
		}
	}
	return 0;
}
