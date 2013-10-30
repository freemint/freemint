/*
 * $Id$
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
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

# ifndef _k_exec_h
# define _k_exec_h

# include "mint/mint.h"

struct create_process_opts
{
	unsigned long mode;
#define CREATE_PROCESS_OPTS_MAXCORE	0x01
#define CREATE_PROCESS_OPTS_NICELEVEL	0x02
#define CREATE_PROCESS_OPTS_DEFDIR	0x04
#define CREATE_PROCESS_OPTS_UID		0x08
#define CREATE_PROCESS_OPTS_GID		0x10
#define CREATE_PROCESS_OPTS_SINGLE 0x20	/* run in single-task */

	long maxcore;
	long nicelevel;
	const char *defdir;
	unsigned short uid;
	unsigned short gid;
};

#if __KERNEL__ == 1

void rts (void); /* XXX */

long _cdecl sys_pexec (short mode, const void *ptr1, const void *ptr2, const void *ptr3);

long _cdecl create_process(const void *ptr1, const void *ptr2, const void *ptr3,
			   struct proc **pret, long stack, struct create_process_opts *);

#endif

# endif /* _k_exec_h */
