
# include "ksocket.h"


long
recvfrom (int fd, void *buf, long buflen, int flags, struct sockaddr *addr, int *addrlen)
{
	struct recvfrom_cmd cmd;
	short addrlen16;
	long r;
	
	if (addrlen)
		addrlen16 = (short)*addrlen;
	
	cmd.cmd		= RECVFROM_CMD;
	cmd.buf		= buf;
	cmd.buflen	= buflen;
	cmd.flags	= flags;
	cmd.addr	= addr;
	cmd.addrlen	= &addrlen16;
	
	r = f_cntl (fd, (long) &cmd, SOCKETCALL);
	
	if (addrlen)
		*addrlen = addrlen16;
	
	return r;
}
