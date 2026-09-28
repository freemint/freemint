/*
 * Copyright 1993 by Ulrich KÅhn. All rights reserved.
 *
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 */

/*
 * File : mount_xdr.c
 *        xdr the structures for the mount protocol
 */


#include <string.h>
#include <sys/types.h>

#include "mount_xdr.h"


bool_t
xdr_dirpath (XDR *x, char *s)
{
	return xdr_string (x, &s, MNTPATHLEN);
}


bool_t
xdr_name (XDR *x, char *s)
{
	return xdr_string (x, &s, MNTNAMLEN);
}


bool_t
xdr_fhandle (XDR *x, fhandle *fhp)
{
	return xdr_opaque (x, fhp->data, MNTFHSIZE);
}


bool_t
xdr_fhstatus (XDR *x, fhstatus *fhsp)
{
	if (!xdr_u_long (x, &fhsp->status))
		return FALSE;
	
	if (0 == fhsp->status)
		return xdr_fhandle (x, &fhsp->fhstatus_u.directory);
	
	return TRUE;
}

long
xdr_size_fhstatus (fhstatus *fhsp)
{
	long r = sizeof (u_long);
	
	if (0 == fhsp->status)
		r += xdr_size_fhandle (&fhsp->fhstatus_u.directory);
	
	return r;
}


bool_t
xdr_mountlist (XDR *x, mountlist *mlp)
{
	char *p = (char *) mlp;
	
	if (XDR_DECODE == x->x_op)
	{
		p += sizeof (mountlist);
		mlp->ml_hostname = p;
	}
	
	if (!xdr_string (x, &mlp->ml_hostname, MNTNAMLEN))
		return FALSE;
	
	if (XDR_DECODE == x->x_op)
	{
		p += strlen (mlp->ml_hostname) + 1;
		mlp->ml_directory = p;
	}
	
	if (!xdr_string (x, &mlp->ml_directory, MNTPATHLEN))
		return FALSE;
	
	if (XDR_DECODE == x->x_op)
	{
		p += strlen (mlp->ml_directory);
		mlp->ml_next = (mountlist *)(((long) p + 1) & (~1L));
	}
	
	return xdr_pointer (x, (char **) &mlp->ml_next, 
	                          sizeof (mountlist), (xdrproc_t) xdr_mountlist);
}

long
xdr_size_mountlist (mountlist *mlp)
{
	long r = 0;

	while (mlp)
	{
		r += 3 * sizeof (u_long);
		r += (strlen (mlp->ml_hostname) + 3) & (~3L);
		r += (strlen (mlp->ml_directory) + 3) & (~3L);
		mlp = mlp->ml_next;
	}
	return r;
}

bool_t
xdr_groups(XDR *x, groups *gp)
{
	char *p = (char *) gp;
	
	if (XDR_DECODE == x->x_op)
	{
		p += sizeof (groups);
		gp->gr_name = p;
	}
	
	if (!xdr_string (x, &gp->gr_name, MNTNAMLEN))
		return FALSE;
	
	if (XDR_DECODE == x->x_op)
	{
		p += strlen (gp->gr_name) + 1;
		gp->gr_next = (groups *)(((long) p + 1) & (~1L));
	}
	
	return xdr_pointer (x, (char **) &gp->gr_next, 
	                          sizeof (groups), (xdrproc_t) xdr_groups);
}

long
xdr_size_groups (groups *gp)
{
	long r = 0;

	while (gp)
	{
		r += 2 * sizeof (u_long);
		r += (strlen (gp->gr_name) + 3) & (~3L);
		gp = gp->gr_next;
	}
	return r;
}

bool_t
xdr_exportlist (XDR *x, exportlist *elp)
{
	char *p = (char *) elp;
	
	if (XDR_DECODE == x->x_op)
	{
		p += sizeof (exportlist);
		elp->ex_filesys = p;
	}
	
	if (!xdr_string (x, &elp->ex_filesys, MNTPATHLEN))
		return FALSE;
	
	if (XDR_DECODE == x->x_op)
	{
		groups *gp = elp->ex_groups;
		
		if (gp)
		{
			while (gp->gr_next)
				gp = gp->gr_next;
			p = gp->gr_name + strlen (gp->gr_name) + 1;
		}
		elp->ex_next = (exportlist *)(((long) p + 1) & (~1L));
	}
	
	return xdr_pointer (x, (char **) &elp->ex_next, 
	                         sizeof (exportlist), (xdrproc_t) xdr_exportlist);
}

long
xdr_size_exportlist (exportlist *elp)
{
	long r = 0;
	
	while (elp)
	{
		r += 3 * sizeof (u_long);
		r += (strlen (elp->ex_filesys) + 3) & (~3L);
		r += xdr_size_groups (elp->ex_groups);
		elp = elp->ex_next;
	}
	
	return r;
}
