/*
 * sendmsg() emulation for MiNT-Net, (w) '93, kay roemer
 *
 * Modified to support Pure-C, Thorsten Otto.
 */

#include "stsocket.h"
#include "mintsock.h"

int sendmsg(int fd, const struct msghdr *msg, int flags)
{
	int r;

#if !defined(MAGIC_ONLY)
	if (__libc_newsockets)
	{
		r = (int)Fsendmsg(fd, msg, flags);

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
		struct sendmsg_cmd cmd;

#ifdef UNIX_DOMAIN_SUPPORT
		struct sockaddr_un un;
		struct sockaddr_un *unp = (struct sockaddr_un *) msg->msg_name;

		if (unp && unp->sun_family == AF_UNIX)
		{
			struct msghdr new_msg;

			if (msg->msg_namelen <= UN_OFFSET || msg->msg_namelen > sizeof(un))
			{
				__set_errno(EINVAL);
				return -1;
			}

			un.sun_family = AF_UNIX;
			_unx2dos(unp->sun_path, un.sun_path, PATH_MAX);
			un.sun_path[sizeof(un.sun_path) - 1] = '\0';

			new_msg.msg_name = &un;
			new_msg.msg_namelen = UN_OFFSET + strlen(un.sun_path);
			new_msg.msg_iov = msg->msg_iov;
			new_msg.msg_iovlen = msg->msg_iovlen;
			new_msg.msg_control = msg->msg_control;
			new_msg.msg_controllen = msg->msg_controllen;

			cmd.msg = &new_msg;
		} else
#endif
		{
			cmd.msg = msg;
		}
		cmd.cmd = SENDMSG_CMD;
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
