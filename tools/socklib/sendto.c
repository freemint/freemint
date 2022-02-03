/*
 * sendto() emulation for MiNT-Net, (w) '93, kay roemer
 *
 * Modified to support Pure-C, Thorsten Otto.
 */

#include "stsocket.h"
#include "mintsock.h"

int sendto(int fd, const void *buf, size_t buflen, int flags, const struct sockaddr *addr, socklen_t addrlen)
{
	int r;

#if !defined(MAGIC_ONLY)
	if (__libc_newsockets)
	{
		r = (int)Fsendto(fd, buf, buflen, flags, addr, addrlen);

		if (r != -ENOSYS)
		{
			if (r < 0)
			{
				__set_errno(-r);
				return -1;
			}
			return r;
		}
		__libc_newsockets = 0;
	}
#endif

	{
		struct sendto_cmd cmd;
		
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
			cmd.addrlen = UN_OFFSET + strlen(un.sun_path);
		} else
#endif
		{
			cmd.addr = addr;
			cmd.addrlen = (short) addrlen;
		}
		cmd.cmd = SENDTO_CMD;
		cmd.buf = buf;
		cmd.buflen = buflen;
		cmd.flags = flags;

		r = (int)Fcntl(fd, (long) &cmd, SOCKETCALL);
		if (r < 0)
		{
			__set_errno(-r);
			return -1;
		}
	}
	return r;
}
