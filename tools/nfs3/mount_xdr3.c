/*
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 *
 * NFS version 3 (RFC 1813) support, derived from the NFS version 2
 * driver. See the file COPYING for copying and using conditions.
 */

/*
 * File : mount_xdr3.c
 *        xdr the structures for version 3 of the mount protocol
 */


#include <string.h>
#include <sys/types.h>

#include "mount_xdr3.h"


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
xdr_fhandle3 (XDR *x, fhandle3 *fhp)
{
	char *p = fhp->data;

	/* variable length now; version 1 had a fixed 32 byte handle */
	return xdr_bytes (x, &p, &fhp->len, FHSIZE3);
}


bool_t
xdr_mountres3 (XDR *x, mountres3 *mrp)
{
	int *p = mrp->auth_flavors;

	if (!xdr_u_int (x, &mrp->status))
		return FALSE;

	if (mrp->status != MNT3_OK)
	{
		mrp->fhandle.len = 0;
		mrp->nauth = 0;
		return TRUE;
	}

	if (!xdr_fhandle3 (x, &mrp->fhandle))
		return FALSE;

	/* the list of accepted auth flavours is new in version 3 */
	return xdr_array (x, (char **) &p, &mrp->nauth, MAX_AUTH_FLAVORS,
			  sizeof (int), (xdrproc_t) xdr_int);
}


const char *
mountstat3_str (u_int status)
{
	switch (status)
	{
		case MNT3_OK:			return "no error";
		case MNT3ERR_PERM:		return "not owner";
		case MNT3ERR_NOENT:		return "no such file or directory";
		case MNT3ERR_IO:		return "I/O error";
		case MNT3ERR_ACCES:		return "permission denied";
		case MNT3ERR_NOTDIR:		return "not a directory";
		case MNT3ERR_INVAL:		return "invalid argument";
		case MNT3ERR_NAMETOOLONG:	return "file name too long";
		case MNT3ERR_NOTSUPP:		return "operation not supported";
		case MNT3ERR_SERVERFAULT:	return "server fault";
	}

	return "unknown error";
}
