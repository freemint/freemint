
# include "ksocket.h"


long
recvmsg (int fd, struct msghdr *msg, int flags)
{
	struct recvmsg_cmd cmd;
	
	cmd.cmd		= RECVMSG_CMD;
	cmd.msg		= msg;
	cmd.flags	= flags;
	
	return f_cntl (fd, (long) &cmd, SOCKETCALL);
}
