
# include "ksocket.h"


long
getsockopt (int fd, int level, int optname, void *optval, long *optlen)
{
	struct getsockopt_cmd cmd;
	long optlen32;
	long r;
	
	if (optlen)
		optlen32 = *optlen;
	
	cmd.cmd		= GETSOCKOPT_CMD;
	cmd.level	= level;
	cmd.optname	= optname;
	cmd.optval	= optval;
	cmd.optlen	= &optlen32;
	
	r = f_cntl (fd, (long) &cmd, SOCKETCALL);
	
	if (optlen)
		*optlen = optlen32;
	
	return r;
}
