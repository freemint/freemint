
# include "ksocket.h"


long
getsockname (int fd, struct sockaddr *addr, long *addrlen)
{
	struct getsockname_cmd cmd;
	short addrlen16;
	long r;
	
	if (addrlen)
		addrlen16 = (short)*addrlen;
	
	cmd.cmd		= GETSOCKNAME_CMD;
	cmd.addr	= addr;
	cmd.addrlen	= &addrlen16;
	
	r = f_cntl (fd, (long)&cmd, SOCKETCALL);
	
	if (addrlen)
		*addrlen = addrlen16;
	
	return r;
}
