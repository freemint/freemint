
# include "ksocket.h"


long
send (int fd, const void *buf, long buflen, int flags)
{
	struct send_cmd cmd;
	
	cmd.cmd		= SEND_CMD;
	cmd.buf		= buf;
	cmd.buflen	= buflen;
	cmd.flags	= flags;
	
	return f_cntl (fd, (long) &cmd, SOCKETCALL);
}
