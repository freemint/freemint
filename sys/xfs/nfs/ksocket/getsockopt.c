/*
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this file; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

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
