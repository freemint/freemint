/*
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
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

int launch(LOCK lock, short mode, short wisgr, short wiscr, char *parm, char *tail, XA_CLIENT *caller);
int copy_env(char *to, char *s[], char *without, char **last);
long count_env(char *s[], char *without);
char *get_env(LOCK lock, const char *name);
long put_env(LOCK lock, short wisgr, short wiscr, char *cmd);
char *shell_find(LOCK lock, XA_CLIENT *client, char *fn);
XA_CLIENT * xa_fork_exec(short x_mode, XSHELW *xsh, char *fname, char *tail);

#endif /* _xa_shel_h */
