
# include "ksocket.h"


long
setsockopt (int fd, int level, int optname, void *optval, long optlen)
{
	struct setsockopt_cmd cmd;
	
	cmd.cmd		= SETSOCKOPT_CMD;
	cmd.level	= level;
	cmd.optname	= optname;
	cmd.optval	= optval;
	cmd.optlen	= optlen;
	
	return f_cntl (fd, (long) &cmd, SOCKETCALL);
}
