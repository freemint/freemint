
# include "ksocket.h"


long
connect (int fd, struct sockaddr *addr, long addrlen)
{
	struct connect_cmd cmd;
	
	cmd.addr	= addr;
	cmd.addrlen	= (short) addrlen;
	cmd.cmd		= CONNECT_CMD;
	
	return f_cntl (fd, (long) &cmd, SOCKETCALL);
}
