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
