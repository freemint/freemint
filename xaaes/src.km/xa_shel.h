/*
 * $Id$
 * 
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann & O.Skancke
 *
 * A multitasking AES replacement for MiNT
 *
 * This file is part of XaAES.
 *
 * XaAES is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * XaAES is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _xa_shel_h
#define _xa_shel_h

#include "global.h"
#include "xa_types.h"

int launch(enum locks lock,
	   short mode, short wisgr, short wiscr,
	   const char *parm, char *tail,
	   struct xa_client *caller);

char *shell_find(enum locks lock, struct xa_client *client, char *fn);
long put_env(enum locks lock, const char *cmd);

char * const * const get_raw_env(void);

AES_function
	XA_shell_write,
	XA_shell_read,
	XA_shell_find,
	XA_shell_envrn;

#endif /* _xa_shel_h */
