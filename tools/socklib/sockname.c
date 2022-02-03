/*
 * getsockname() emulation for MiNT-Net, (w) '93, kay roemer
 *
 * Modified to support Pure-C, Thorsten Otto.
 */

#include "stsocket.h"
#include "mintsock.h"


int getsockname(int fd, struct sockaddr *addr, socklen_t *addrlen)
{
	int r;

#if !defined(MAGIC_ONLY)
	if (__libc_newsockets)
	{
		r = (int)Fgetsockname(fd, addr, addrlen);

		if (r != -ENOSYS)
		{
			if (r < 0)
			{
				__set_errno(-r);
				return -1;
			}
			return 0;
		}
		__libc_newsockets = 0;
	}
#endif

	{
		struct getsockname_cmd cmd;
		short addrlen16;

		if (addrlen)
			addrlen16 = (short) *addrlen;

		cmd.cmd = GETSOCKNAME_CMD;
		cmd.addr = addr;
		cmd.addrlen = &addrlen16;

		r = (int)Fcntl(fd, (long) &cmd, SOCKETCALL);

#if defined(UNIX_DOMAIN_SUPPORT) && 0
		if (addrlen)
		{
			short oaddrlen;

			oaddrlen = *addrlen;

			if (addr && addr->sa_family == AF_UNIX && r >= 0)
			{
				struct sockaddr_un *unp = (struct sockaddr_un *) addr;
				char name[sizeof(unp->sun_path)];

				if (addrlen16 > UN_OFFSET)
				{
					_dos2unx(unp->sun_path, name, PATH_MAX);
					addrlen16 = UN_OFFSET;
					addrlen16 += _sncpy(unp->sun_path, name, oaddrlen - UN_OFFSET);
				}
			}
		}
#endif

		if (addrlen)
			*addrlen = addrlen16;

		if (r < 0)
		{
			__set_errno(-r);
			return -1;
		}
	}
	return 0;
}


