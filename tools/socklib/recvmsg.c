/*
 * recvmsg() emulation for MiNT-Net, (w) '93, kay roemer
 *
 * Modified to support Pure-C, Thorsten Otto.
 */

#include "stsocket.h"
#include "mintsock.h"

int recvmsg(int fd, struct msghdr *msg, int flags)
{
	int r;

#if !defined(MAGIC_ONLY)
	if (__libc_newsockets)
	{
		r = (int)Frecvmsg(fd, msg, flags);

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
		struct recvmsg_cmd cmd;
#if defined(UNIX_DOMAIN_SUPPORT) && 0
		struct sockaddr_un *unp = (struct sockaddr_un *) msg->msg_name;
		short oaddrlen = msg->msg_namelen;
#endif

		cmd.cmd = RECVMSG_CMD;
		cmd.msg = msg;
		cmd.flags = flags;

		r = (int)Fcntl(fd, (long) &cmd, SOCKETCALL);

#if defined(UNIX_DOMAIN_SUPPORT) && 0
		if (unp && unp->sun_family == AF_UNIX && r >= 0)
		{
			char name[sizeof(unp->sun_path)];

			if (msg->msg_namelen > UN_OFFSET)
			{
				_dos2unx(unp->sun_path, name, PATH_MAX);
				msg->msg_namelen = UN_OFFSET;
				msg->msg_namelen += _sncpy(unp->sun_path, name, oaddrlen - UN_OFFSET);
			}
		}
#endif

		if (r < 0)
		{
			__set_errno(-r);
			return -1;
		}
	}
	return r;
}
