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

# ifndef _libsocket_h
# define _libsocket_h

/*
 * raw prototypes
 */

void getfunc_unlock (void);
struct hostent * _cdecl st_gethostbyname ();
struct hostent * _cdecl st_gethostbyaddr ();
struct hostent * _cdecl stbl_gethostbyname ();
struct hostent * _cdecl stbl_gethostbyaddr ();
short            _cdecl st_gethostname	 ();
struct servent * _cdecl st_getservbyname ();
struct servent * _cdecl st_getservbyport ();
struct servent * _cdecl stbl_getservbyname ();
struct servent * _cdecl stbl_getservbyport ();

# endif /* _libsocket_h */
