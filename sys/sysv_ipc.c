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

# include "sysv_ipc.h"

# include "mint/stat.h"

# include "k_prot.h"


/*
 * Check for ipc permission
 */

long
ipcperm (struct ucred *cred, struct ipc_perm *perm, long mode)
{
	long mask = 0;
	
	if (suser (cred))
		return 0;
	
	if (mode == IPC_M)
	{
		if (cred->euid == perm->uid || cred->euid == perm->cuid)
			return 0;
		
		return EPERM;
	}
	
	if (cred->euid == perm->uid || cred->euid == perm->cuid)
	{
		if (mode & IPC_R)
			mask |= S_IRUSR;
		if (mode & IPC_W)
			mask |= S_IWUSR;
		
		return ((perm->mode & mask) == mask ? 0 : EACCES);
	}
	
	if (cred->egid == perm->gid || groupmember (cred, perm->gid) ||
	    cred->egid == perm->cgid || groupmember (cred, perm->cgid))
	{
		if (mode & IPC_R)
			mask |= S_IRGRP;
		if (mode & IPC_W)
			mask |= S_IWGRP;
		
		return ((perm->mode & mask) == mask ? 0 : EACCES);
	}
	
	if (mode & IPC_R)
		mask |= S_IROTH;
	if (mode & IPC_W)
		mask |= S_IWOTH;
	
	return ((perm->mode & mask) == mask ? 0 : EACCES);
}
