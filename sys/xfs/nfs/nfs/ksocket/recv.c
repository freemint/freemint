
# include "ksocket.h"


long
recv (int fd, void *buf, long buflen, int flags)
{
	struct recv_cmd cmd;
	
	cmd.cmd		= RECV_CMD;
	cmd.buf		= buf;
	cmd.buflen	= buflen;
	cmd.flags	= flags;
	
	return f_cntl (fd, (long) &cmd, SOCKETCALL);
}
