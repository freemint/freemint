
# include "ksocket.h"


long
sendmsg (int fd, const struct msghdr *msg, int flags)
{
	struct sendmsg_cmd cmd;
	
	cmd.msg		= msg;
	cmd.cmd		= SENDMSG_CMD;
	cmd.flags	= flags;
	
	return f_cntl (fd, (long) &cmd, SOCKETCALL);
}
