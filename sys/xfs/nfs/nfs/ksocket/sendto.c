
# include "ksocket.h"


long
sendto (int fd, const void *buf, long buflen, int flags, const struct sockaddr *addr, int addrlen)
{
	struct sendto_cmd cmd;
	
	cmd.addr	= addr;
	cmd.addrlen	= (short) addrlen;
	cmd.cmd		= SENDTO_CMD;
	cmd.buf		= buf;
	cmd.buflen	= buflen;
	cmd.flags	= flags;
	
	return f_cntl (fd, (long) &cmd, SOCKETCALL);
}
