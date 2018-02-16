/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 1999, 2000, 2001 Frank Naumann <fnaumann@freemint.de>
 * Copyright 1999 Jens Heitmann
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * 
 * Author: Frank Naumann, Jens Heitmann
 * Started: 1999-05
 * 
 * please send suggestions or bug reports to me or
 * the MiNT mailing list
 * 
 */

# ifndef _syscalls_h
# define _syscalls_h

/*
 * raw prototypes
 */

short	_cdecl st_socket	();
short	_cdecl st_close 	();
short	_cdecl st_seek		();
short 	_cdecl st_fcntl 	();

size_t 	_cdecl st_read		();
size_t 	_cdecl st_write 	();

size_t 	_cdecl st_recv		();
size_t 	_cdecl st_recvfrom	();
size_t 	_cdecl st_recvmsg 	(); 				 
size_t 	_cdecl st_send		();
size_t 	_cdecl st_sendto	();
size_t 	_cdecl st_sendmsg 	();

short	_cdecl st_accept 	();
short	_cdecl st_bind		();
short	_cdecl st_connect 	();
short	_cdecl st_listen 	();
short	_cdecl st_shutdown	();

short	_cdecl st_getsockname 	();
short	_cdecl st_getpeername 	();

short	_cdecl st_getsockopt	();
short	_cdecl st_setsockopt	();

short	_cdecl st_select 	(); 								


# endif /* _syscalls_h */
