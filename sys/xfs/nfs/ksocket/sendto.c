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
sendto (int fd, const void *buf, long buflen, int flags, const struct sockaddr *addr, int addrlen)
{
	struct sendto_cmd cmd;
	
	cmd.addr	= addr;
	cmd.addrlen	= (short) addrlen;
	cmd.cmd		= SENDTO_CMD;
	cmd.buf		= buf;
	cmd.buflen	= buflen;
	cmd.flags	= flags;
	
	return f_cntl (fd, (long) &cmd, SOCKETCALL);
}
