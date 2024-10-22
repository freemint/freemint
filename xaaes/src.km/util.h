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

#ifndef _util_h
#define _util_h

#include "global.h"

int drive_from_letter(int c);
int letter_from_drive(int d);
int get_drv(const char *p);
void fix_path(char *path);
void strip_fname(const char *filename, char *pn, char *fn);
int drive_and_path(char *fname, char *path, char *name, bool n, bool set);
void set_drive_and_path(char *fname);
void get_drive_and_path(char *path, short plen);
short xa_toupper( short c );
char *xa_strdup(char*s);
void strnupr(char *s, int n);

typedef struct xa_file XA_FILE;

XA_FILE *xa_fopen(const char *fn, int rwmd);
void xa_fclose( XA_FILE *fp );
long xa_rewind( XA_FILE *fp );
int xa_writeline(const char *buf, long l, XA_FILE *fp);
char *xa_readline(char *buf, long size, XA_FILE *fp);
#endif /* _util_h */
