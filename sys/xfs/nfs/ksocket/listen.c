
# include "ksocket.h"


long
listen (int fd, int backlog)
{
	struct listen_cmd cmd;
	
	cmd.cmd		= LISTEN_CMD;
	cmd.backlog	= backlog;
	
	return f_cntl (fd, (long) &cmd, SOCKETCALL);
}
