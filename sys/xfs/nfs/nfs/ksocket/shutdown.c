
# include "ksocket.h"


long
shutdown (int fd, int how)
{
	struct shutdown_cmd cmd;
	
	cmd.cmd = SHUTDOWN_CMD;
	cmd.how = how;
	
	return f_cntl (fd, (long) &cmd, SOCKETCALL);
}
