/*
 * Filename:     
 * Version:      
 * Author:       Frank Naumann
 * Started:      1999-05
 * Last Updated: 2001-04-17
 * Target O/S:   FreeMiNT 1.16
 * Description:  
 * 
 * Note:         Please send suggestions, patches or bug reports to me
 *               or the MiNT mailing list <mint@fishpool.com>.
 * 
 * Copying:      Copyright 1999, 200, 2001 Frank Naumann <fnaumann@freemint.de>
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
