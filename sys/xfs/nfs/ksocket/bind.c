
# include "ksocket.h"

long
bind (int fd, struct sockaddr *addr, long addrlen)
{
	struct bind_cmd cmd;
	
	cmd.addr	= addr;
	cmd.addrlen	= addrlen;
	cmd.cmd		= BIND_CMD;
	
	return f_cntl (fd, (long) &cmd, SOCKETCALL);
}
