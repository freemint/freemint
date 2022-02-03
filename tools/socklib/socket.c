/*
 * socket() emulation for MiNT-Net, (w) '93, kay roemer.
 *
 * Modified to support Pure-C, Thorsten Otto.
 */

#include "stsocket.h"
#include "mintsock.h"

int socket(int domain, int type, int proto)
{
	int r;

#if !defined(MAGIC_ONLY)
	if (__libc_newsockets)
	{
		r = (int)Fsocket(domain, type, proto);

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
		struct socket_cmd cmd;
		int sockfd;

		sockfd = (int)Fopen(SOCKDEV, O_RDWR);
		if (sockfd < 0)
		{
			__set_errno(-sockfd);
			return -1;
		}

		cmd.cmd = SOCKET_CMD;
		cmd.domain = domain;
		cmd.type = type;
		cmd.protocol = proto;

		r = (int)Fcntl(sockfd, (long) &cmd, SOCKETCALL);
		if (r < 0)
		{
			__set_errno(-r);
			Fclose(sockfd);
			return -1;
		}

		return sockfd;
	}
}


