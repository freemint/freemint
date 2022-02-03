/*
 * recv() emulation for MiNT-Net, (w) '93, kay roemer
 *
 * Modified to support Pure-C, Thorsten Otto.
 */

#include "stsocket.h"
#include "mintsock.h"

int recv(int fd, void *buf, size_t buflen, int flags)
{
	int r;

#if !defined(MAGIC_ONLY)
	if (__libc_newsockets)
	{
		r = (int)Frecvfrom(fd, buf, buflen, flags, NULL, NULL);

		if (r != -ENOSYS)
		{
			if (r < 0)
			{
				__set_errno(-(int)r);
				return -1;
			}
			return (int)r;
		}
		__libc_newsockets = 0;
	}
#endif

	{
		struct recv_cmd cmd;

		cmd.cmd = RECV_CMD;
		cmd.buf = buf;
		cmd.buflen = buflen;
		cmd.flags = flags;

		r = (int)Fcntl(fd, (long) &cmd, SOCKETCALL);
		if (r < 0)
		{
			__set_errno(-(int)r);
			return -1;
		}
	}
	return (int)r;
}
