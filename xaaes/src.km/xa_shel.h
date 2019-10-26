/*
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann & O.Skancke
 *
 * A multitasking AES replacement for FreeMiNT
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

int launch(int lock,
	   short mode, short wisgr, short wiscr,
	   const char *parm, char *tail,
	   struct xa_client *caller);

char *shell_find(int lock, struct xa_client *client, char *fn);

void init_env(void);

long put_env(int lock, const char *cmd);
const char *get_env(int lock, const char *name);

// char * const * const get_raw_env(void);
const char ** get_raw_env(void);

AES_function
	XA_shel_write,
	XA_shel_read,
	XA_shel_find,
	XA_shel_envrn,
	XA_shel_help;

#endif /* _xa_shel_h */
