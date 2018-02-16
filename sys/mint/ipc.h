/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * 
 * Author: Frank Naumann <fnaumann@freemint.de>
 * Started: 2000-12-06
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _mint_ipc_h
# define _mint_ipc_h

# include "ktypes.h"


struct ipc_perm
{
	short		uid;	/* user id */
	short		gid;	/* group id */
	short		cuid;	/* creator user id */
	short		cgid;	/* creator group id */
	long		mode;	/* r/w permission */
	
	/*
	 * These members are private and used only in the internal
	 * implementation of this interface.
	 */
	ulong		_seq;	/* sequence */
	long		_key;	/* user specified msg/sem/shm key */
};

# define IPC_CREAT	001000	/* create entry if key does not exist */
# define IPC_EXCL	002000	/* fail if key exists */
# define IPC_NOWAIT	004000	/* error if request must wait */

# define IPC_PRIVATE	0	/* private key */

# define IPC_RMID	0	/* remove identifier */
# define IPC_SET	1	/* set options */
# define IPC_STAT	2	/* get options */


# endif /* _mint_ipc_h */
