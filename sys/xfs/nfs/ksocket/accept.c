
# include "ksocket.h"


long
accept (int fd, struct sockaddr *addr, long *addrlen)
{
	struct accept_cmd cmd;
	short addrlen16;
	long newsock;
	
	if (addrlen)
		addrlen16 = (short) *addrlen;
	
	cmd.cmd		= ACCEPT_CMD;
	cmd.addr	= addr;
	cmd.addrlen	= &addrlen16;
	
	newsock = f_cntl (fd, (long) &cmd, SOCKETCALL);
	
	if (addrlen)
		*addrlen = addrlen16;
	
	return newsock;
}
