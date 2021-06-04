/*
 * socketpair() emulation for MiNT-Net, (w) '93, kay roemer.
 *
 * Modified to support Pure-C, Thorsten Otto.
 */

#include "stsocket.h"
#include "mintsock.h"

int socketpair(int domain, int type, int proto, int fds[2])
{
#if !defined(MAGIC_ONLY)
	if (__libc_newsockets)
	{
		short _fds[2];
		int r = (int)Fsocketpair(domain, type, proto, _fds);

		if (r != -ENOSYS)
		{
			if (r < 0)
			{
				__set_errno(-r);
				return -1;
			}
			fds[0] = _fds[0];
			fds[1] = _fds[1];
			return 0;
		}
		__libc_newsockets = 0;
	}
#endif

	{
		struct socketpair_cmd cmd;
		int sockfd1;
		int sockfd2;

		sockfd1 = (int)Fopen(SOCKDEV, O_RDWR);
		if (sockfd1 < 0)
		{
			__set_errno(-sockfd1);
			return -1;
		}

		cmd.cmd = SOCKETPAIR_CMD;
		cmd.domain = domain;
		cmd.type = type;
		cmd.protocol = proto;

		sockfd2 = (int)Fcntl(sockfd1, (long) &cmd, SOCKETCALL);
		if (sockfd2 < 0)
		{
			__set_errno(-sockfd2);
			Fclose(sockfd1);
			return -1;
		}

		fds[0] = sockfd1;
		fds[1] = sockfd2;
	}
	return 0;
}
