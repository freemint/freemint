
# include "ksocket.h"


long
socketpair (int domain, int type, int proto, int fds[2])
{
	struct socketpair_cmd cmd;
	int sockfd1, sockfd2;
	
	sockfd1 = f_open (SOCKDEV, O_RDWR);
	if (sockfd1 < 0)
		return sockfd1;
	
	cmd.cmd		= SOCKETPAIR_CMD;
	cmd.domain	= domain;
	cmd.type	= type;
	cmd.protocol	= proto;
	
	sockfd2 = f_cntl (sockfd1, (long) &cmd, SOCKETCALL);
	if (sockfd2 < 0)
	{
		f_close (sockfd1);
		return sockfd2;
	}
	
	fds[0] = sockfd1;
	fds[1] = sockfd2;
	
	return 0;
}
