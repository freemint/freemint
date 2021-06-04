/*
 * send() emulation for MiNT-Net, (w) '93, kay roemer
 *
 * Modified to support Pure-C, Thorsten Otto.
 */

#include "stsocket.h"
#include "mintsock.h"

int send(int fd, const void *buf, size_t buflen, int flags)
{
	int r;

#if !defined(MAGIC_ONLY)
	if (__libc_newsockets)
	{
		r = (int)Fsendto(fd, buf, buflen, flags, NULL, 0);

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
		struct send_cmd cmd;

		cmd.cmd = SEND_CMD;
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


