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
