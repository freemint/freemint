/*
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

# include "k_prot.h"

# include "libkern/libkern.h"
# include "mint/proc.h"

# include "kmemory.h"

# include "proc.h"


long _cdecl
sys_pgetuid(void)
{
	return get_curproc()->p_cred->ruid;
}

long _cdecl
sys_pgetgid(void)
{
	return get_curproc()->p_cred->rgid;
}

long _cdecl
sys_pgeteuid(void)
{
	return get_curproc()->p_cred->ucr->euid;
}

long _cdecl
sys_pgetegid(void)
{
	return get_curproc()->p_cred->ucr->egid;
}

long _cdecl
proc_setuid(struct proc *p, unsigned short uid)
{
	struct pcred *cred = p->p_cred;
	
	if (cred->ruid != uid && !suser(cred->ucr))
		return EACCES; /* XXX EPERM */
	
	if (cred->ruid == uid && cred->suid == uid && cred->ucr->euid == uid)
		return uid;
	
	cred->ucr = copy_cred(cred->ucr);
	cred->ucr->euid = uid;
	cred->ruid = uid;
	cred->suid = uid;
	
	return uid;
}

long _cdecl
sys_psetuid(unsigned short uid)
{
	TRACE(("Psetuid(%i)", uid));
	return proc_setuid(get_curproc(), uid);
}

long _cdecl
proc_setgid(struct proc *p, unsigned short gid)
{
	struct pcred *cred = p->p_cred;
	
	if (cred->rgid != gid && !suser(cred->ucr))
		return EACCES; /* XXX EPERM */
	
	if (cred->rgid == gid && cred->ucr->egid == gid && cred->sgid == gid)
		return gid;
	
	cred->ucr = copy_cred(cred->ucr);
	cred->ucr->egid = gid;
	cred->rgid = gid;
	cred->sgid = gid;
	
	return gid;
}

long _cdecl
sys_psetgid(unsigned short gid)
{
	TRACE(("Psetgid(%i)", gid));
	return proc_setgid(get_curproc(), gid);
}

/* uk, blank: set effective uid/gid but leave the real uid/gid unchanged. */
long _cdecl
sys_psetreuid(unsigned short ruid, unsigned short euid)
{
	struct pcred *cred = get_curproc()->p_cred;
	
	TRACE(("Psetreuid(%i, %i)", ruid, euid));
	
	if (ruid != -1 &&
	    ruid != cred->ruid && ruid != cred->ucr->euid &&
	    !suser(cred->ucr))
		return EACCES; /* XXX EPERM */
	
	if (euid != -1 &&
	    euid != cred->ruid && euid != cred->ucr->euid &&
	    euid != cred->suid &&
	    !suser(cred->ucr))
		return EACCES; /* XXX EPERM */
	
	if (euid != -1 && euid != cred->ucr->euid)
	{
		cred->ucr = copy_cred(cred->ucr);
		cred->ucr->euid = euid;
	}
	
	if (ruid != -1 && (cred->ruid != ruid || cred->suid != cred->ucr->euid))
	{
		cred->ruid = ruid;
		cred->suid = cred->ucr->euid;
	}

	return E_OK;
}
	
long _cdecl
sys_psetregid(unsigned short rgid, unsigned short egid)
{
	struct pcred *cred = get_curproc()->p_cred;
	
	TRACE(("Psetregid(%i, %i)", rgid, egid));
	
	if (rgid != -1 &&
	    rgid != cred->rgid && rgid != cred->ucr->egid &&
	    !suser(cred->ucr))
		return EACCES; /* XXX EPERM */
	
	if (egid != -1 &&
	    egid != cred->rgid && egid != cred->ucr->egid &&
	    egid != cred->sgid &&
	    !suser(cred->ucr))
		return EACCES; /* XXX EPERM */
	
	if (egid != -1 && cred->ucr->egid != egid)
	{
		cred->ucr = copy_cred(cred->ucr);
		cred->ucr->egid = egid;
	}
	
	if (rgid != -1 &&
	    (cred->rgid != rgid || cred->sgid != cred->ucr->egid))
	{
		cred->rgid = rgid;
		cred->sgid = cred->ucr->egid;
	}

	return E_OK;
}

long _cdecl
sys_pseteuid(unsigned short euid)
{
	if (sys_psetreuid(-1, euid) == 0)
		return euid;

	return EACCES; /* XXX EPERM */
}
	
long _cdecl
sys_psetegid(unsigned short egid)
{
	if (sys_psetregid(-1, egid) == 0)
		return egid;

	return EACCES; /* XXX EPERM */
}

/* tesche: get/set supplemantary group id's.
 */
long _cdecl
sys_pgetgroups(short gidsetlen, unsigned short gidset[])
{
	struct ucred *cred = get_curproc()->p_cred->ucr;
	int i;
	
	TRACE(("Pgetgroups(%i, ...)", gidsetlen));
	
	if (gidsetlen == 0)
		return cred->ngroups;
	
	if (gidsetlen < cred->ngroups)
		return EBADARG;
	
	for (i = 0; i < cred->ngroups; i++)
		gidset[i] = cred->groups[i];
	
	return cred->ngroups;
}

long _cdecl
sys_psetgroups(short ngroups, unsigned short gidset[])
{
	struct pcred *cred = get_curproc()->p_cred;
	ushort grsize;
	
	TRACE(("Psetgroups(%i, ...)", ngroups));
	
	if (!suser(cred->ucr))
		return EACCES; /* XXX EPERM */
	
	if (ngroups < 0 || ngroups > NGROUPS)
		return EBADARG;
	
	grsize = ngroups * sizeof(gidset[0]);
	if (cred->ucr->ngroups == ngroups &&
	    memcmp(gidset, cred->ucr->groups, grsize) == 0)
		return ngroups;
	
	cred->ucr = copy_cred(cred->ucr);
	
	cred->ucr->ngroups = ngroups;
	memcpy(cred->ucr->groups, gidset, grsize);
	
	return ngroups;
}

/* utility functions */

int
groupmember (struct ucred *cred, ushort group)
{
	int i;
	
	for (i = 0; i < cred->ngroups; i++)
	{
		if (cred->groups[i] == group)
			return 1;
	}
	
	return 0;
}

/* p_cred */

struct ucred *
copy_cred(struct ucred *ucr)
{
	struct ucred *n;
	
	TRACE(("copy_cred: %p links %i", ucr, ucr->links));
	assert(ucr->links > 0);
	
	if (ucr->links == 1)
		return ucr;
	
	n = kmalloc(sizeof(*n));
	if (n)
	{
		/* copy */
		*n = *ucr;
		
		/* adjust link counters */
		ucr->links--;
		n->links = 1;
	}
	else
		DEBUG(("copy_cred: kmalloc failed -> NULL"));
	
	return n;
}

void
free_cred(struct ucred *ucr)
{
	assert(ucr->links > 0);
	
	if (--ucr->links == 0)
		kfree(ucr);
}
