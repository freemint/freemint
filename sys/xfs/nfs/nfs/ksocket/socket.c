
# include "ksocket.h"


long
socket (int domain, int type, int proto)
{
	struct socket_cmd cmd;
	int sockfd, r;
	
	sockfd = f_open (SOCKDEV, O_RDWR|O_GLOBAL);
	if (sockfd < 0)
		return sockfd;
	
	cmd.cmd		= SOCKET_CMD;
	cmd.domain	= domain;
	cmd.type	= type;
	cmd.protocol	= proto;
	
	r = f_cntl (sockfd, (long) &cmd, SOCKETCALL);
	if (r < 0)
	{
		f_close (sockfd);
		return r;
	}
	
	return sockfd;
}
