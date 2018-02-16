/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000-2005 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 */

#ifndef _syscalldefs_h
#define _syscalldefs_h

#define STRMAX 255

struct systab
{
	struct syscall **table;
	int size;
	int max;
	int maxused;
};

struct syscall
{
	char name[STRMAX];
	struct arg *ret;
	struct arg *args;
	int status;
#define SYSCALL_REGULAR		0
#define SYSCALL_UNIMPLEMENTED	1
#define SYSCALL_UNSUPPORTED	2
#define SYSCALL_PASSTHROUGH	3
#define SYSCALL_UNDEFINED	4
};

struct arg
{
	struct arg *next;
	
	int type;
#define TYPE_VOID	0
#define TYPE_INT	1
#define TYPE_CHAR	2
#define TYPE_SHORT	3
#define TYPE_LONG	4
#define TYPE_UNSIGNED	5
#define TYPE_UCHAR	6
#define TYPE_USHORT	7
#define TYPE_ULONG	8
#define TYPE_IDENT	9
	int flags;
#define FLAG_CONST	0x01
#define FLAG_STRUCT	0x02
#define FLAG_UNION	0x04
#define FLAG_POINTER	0x08
#define FLAG_ARRAY	0x10
#define FLAG_POINTER2	0x20
	int ar_size;
	
	char types[STRMAX];
	char name[STRMAX];
};

#endif /* _syscalldefs_h */
