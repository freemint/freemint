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
 * begin:	2000-10-31
 * last change:	2000-10-31
 * 
 * Author:	Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# include "k_prot.h"

# include "libkern/libkern.h"
# include "mint/proc.h"

# include "kmemory.h"


long _cdecl
sys_pgetuid (void)
{
	return curproc->p_cred->ruid;
}

long _cdecl
sys_pgetgid (void)
{
	return curproc->p_cred->rgid;
}

long _cdecl
sys_pgeteuid (void)
{
	return curproc->p_cred->ucr->euid;
}

long _cdecl
sys_pgetegid (void)
{
	return curproc->p_cred->ucr->egid;
}

long _cdecl
sys_psetuid (int uid)
{
	struct pcred *cred = curproc->p_cred;
	
	TRACE (("Psetuid(%i)", uid));
	
# if 1
	if (cred->ruid != uid && !suser (cred->ucr))
		return EACCES; /* XXX EPERM */
	
	if (cred->ruid == uid && cred->suid == uid && cred->ucr->euid == uid)
		return uid;
	
	cred->ucr = copy_cred (cred->ucr);
	cred->ucr->euid = uid;
	cred->ruid = uid;
	cred->suid = uid;
# else
	if (cred->ucr->euid == 0)
		cred->ruid = cred->ucr->euid = cred->suid = uid;
	else if ((uid == cred->ruid) || (uid == cred->suid))
		cred->ucr->euid = uid;
	else
		return EACCES; /* XXX EPERM */
# endif
	
	return uid;
}

long _cdecl
sys_psetgid (int gid)
{
	struct pcred *cred = curproc->p_cred;
	
	TRACE (("Psetgid(%i)", gid));
	
# if 1
	if (cred->rgid != gid && !suser (cred->ucr))
		return EACCES; /* XXX EPERM */
	
	if (cred->rgid == gid && cred->ucr->egid == gid && cred->sgid == gid)
		return gid;
	
	cred->ucr = copy_cred (cred->ucr);
	cred->ucr->egid = gid;
	cred->rgid = gid;
	cred->sgid = gid;
# else
	if (cred->ucr->euid == 0)
		cred->rgid = cred->ucr->egid = cred->sgid = gid;
	else if ((gid == cred->rgid) || (gid == cred->sgid))
		cred->ucr->egid = gid;
	else
		return EACCES; /* XXX EPERM */
# endif

	return gid;
}

/* uk, blank: set effective uid/gid but leave the real uid/gid unchanged. */
long _cdecl
sys_psetreuid (int ruid, int euid)
{
	struct pcred *cred = curproc->p_cred;
	
# if 1
	TRACE (("Psetreuid(%i, %i)", ruid, euid));
	
	if (ruid != -1 &&
	    ruid != cred->ruid && ruid != cred->ucr->euid &&
	    !suser (cred->ucr))
		return EACCES; /* XXX EPERM */
	
	if (euid != -1 &&
	    euid != cred->ruid && euid != cred->ucr->euid &&
	    euid != cred->suid &&
	    !suser (cred->ucr))
		return EACCES; /* XXX EPERM */
	
	if (euid != -1 && euid != cred->ucr->euid)
	{
		cred->ucr = copy_cred (cred->ucr);
		cred->ucr->euid = euid;
	}
	
	if (ruid != -1 && (cred->ruid != ruid || cred->suid != cred->ucr->euid))
	{
		cred->ruid = ruid;
		cred->suid = cred->ucr->euid;
	}
# else
	short old_ruid = cred->ruid;

	TRACE (("Psetreuid(%i, %i)", ruid, euid));
	
	if (ruid != -1)
	{
		if (cred->ucr->euid == ruid || old_ruid == ruid || cred->ucr->euid == 0)
			cred->ruid = ruid;
		else
			return EACCES; /* XXX EPERM */
	}

	if (euid != -1)
	{
		if (cred->ucr->euid == euid || old_ruid == euid || cred->suid == euid || cred->ucr->euid == 0)
			cred->ucr->euid = euid;
		else
		{
			cred->ruid = old_ruid;
			return EACCES; /* XXX EPERM */
		}
	}

	if (ruid != -1 || (euid != -1 && euid != old_ruid))
		cred->suid = cred->ucr->euid;
# endif

	return E_OK;
}
	
long _cdecl
sys_psetregid (int rgid, int egid)
{
	struct pcred *cred = curproc->p_cred;
	
# if 1
	TRACE (("Psetregid(%i, %i)", rgid, egid));
	
	if (rgid != -1 &&
	    rgid != cred->rgid && rgid != cred->ucr->egid &&
	    !suser (cred->ucr))
		return EACCES; /* XXX EPERM */
	
	if (egid != -1 &&
	    egid != cred->rgid && egid != cred->ucr->egid &&
	    egid != cred->sgid &&
	    !suser (cred->ucr))
		return EACCES; /* XXX EPERM */
	
	if (egid != -1 && cred->ucr->egid != egid)
	{
		cred->ucr = copy_cred (cred->ucr);
		cred->ucr->egid = egid;
	}
	
	if (rgid != -1 &&
	    (cred->rgid != rgid || cred->sgid != cred->ucr->egid))
	{
		cred->rgid = rgid;
		cred->sgid = cred->ucr->egid;
	}
# else
	short old_rgid = cred->rgid;

	TRACE (("Psetregid(%i, %i)", rgid, egid));
	
	if (rgid != -1)
	{
		if ((cred->ucr->egid == rgid) || (old_rgid == rgid) || (cred->ucr->euid == 0))
			cred->rgid = rgid;
		else
			return EACCES; /* XXX EPERM */
	}

	if (egid != -1)
	{
		if ((cred->ucr->egid == egid) || (old_rgid == egid) || (cred->sgid == egid) || (cred->ucr->euid == 0))
			cred->ucr->egid = egid;
		else
		{
			cred->rgid = old_rgid;
			return EACCES; /* XXX EPERM */
		}
	}

	if (rgid != -1 || (egid != -1 && egid != old_rgid))
		cred->sgid = cred->ucr->egid;
# endif

	return E_OK;
}

long _cdecl
sys_pseteuid (int euid)
{
	if (sys_psetreuid (-1, euid) == 0)
		return euid;

	return EACCES; /* XXX EPERM */
}
	
long _cdecl
sys_psetegid (int egid)
{
	if (sys_psetregid (-1, egid) == 0)
		return egid;

	return EACCES; /* XXX EPERM */
}

/* tesche: get/set supplemantary group id's.
 */
long _cdecl
sys_pgetgroups (int gidsetlen, int gidset[])
{
	struct ucred *cred = curproc->p_cred->ucr;
	int i;
	
	TRACE (("Pgetgroups(%i, ...)", gidsetlen));
	
	if (gidsetlen == 0)
		return cred->ngroups;
	
	if (gidsetlen < cred->ngroups)
		return EBADARG;
	
	for (i = 0; i < cred->ngroups; i++)
		gidset[i] = cred->groups[i];
	
	return cred->ngroups;
}

long _cdecl
sys_psetgroups (int ngroups, int gidset[])
{
# if 1
	struct pcred *cred = curproc->p_cred;
	ushort grsize;
	
	TRACE (("Psetgroups(%i, ...)", ngroups));
	
	if (!suser (cred->ucr))
		return EACCES; /* XXX EPERM */
	
	if (ngroups < 0 || ngroups > NGROUPS)
		return EBADARG;
	
	grsize = ngroups * sizeof (gidset[0]);
	if (cred->ucr->ngroups == ngroups &&
	    memcmp (gidset, cred->ucr->groups, grsize) == 0)
		return ngroups;
	
	cred->ucr = copy_cred (cred->ucr);
	
	cred->ucr->ngroups = ngroups;
	memcpy (cred->ucr->groups, gidset, grsize);
	
	return ngroups;
# else
	struct ucred *cred = curproc->p_cred->ucr;
	int i;

	TRACE (("Psetgroups(%i, ...)", ngroups));
	
	if (cred->euid)
		return EACCES;	/* only superuser may change this */

	if ((ngroups < 0) || (ngroups > NGROUPS))
		return EBADARG;

	cred->ngroups = ngroups;
	for (i = 0; i < ngroups; i++)
		cred->groups[i] = gidset[i];

	return ngroups;
# endif
}

/* utility functions */

int
ngroupmatch (struct ucred *cred, ushort group)
{
	int i;
	
	for (i = 0; i < cred->ngroups; i++)
		if (cred->groups[i] == group)
			return 1;
	
	return 0;
}

/* p_cred */

struct ucred *
copy_cred (struct ucred *ucr)
{
	struct ucred *n;
	
	TRACE (("copy_cred: %lx links %li", ucr, ucr->links));
	assert (ucr->links > 0);
	
	if (ucr->links == 1)
		return ucr;
	
	n = kmalloc (sizeof (*n));
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
free_cred (struct ucred *ucr)
{
	assert (ucr->links > 0);
	
	if (--ucr->links == 0)
		kfree (ucr);
}
