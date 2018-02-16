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
 * Started: 2000-12-05
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _sysv_ipc_h
# define _sysv_ipc_h

# include "mint/mint.h"
# include "mint/credentials.h"
# include "mint/ipc.h"


# define IPCID_TO_IX(id)	((id) & 0xffff)
# define IPCID_TO_SEQ(id)	(((id) >> 16) & 0xffff)

/* Common access type bits, used with ipcperm(). */
# define IPC_R			000400	/* read permission */
# define IPC_W			000200	/* write/alter permission */
# define IPC_M			010000	/* permission to change control info */

long ipcperm (struct ucred *cred, struct ipc_perm *perm, long mode);


# endif	/* _sysv_ipc_h  */
