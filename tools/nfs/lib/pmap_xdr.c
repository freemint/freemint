/*
 * Copyright 1993, 1994 by Ulrich Khn. All rights reserved.
 *
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 */

/*
 * File : pmap_xdr.c
 *        xdr port mapper structures
 */

#include "xdr.h"
#include "pmap.h"


bool_t
xdr_pmap(xdrs *x, struct pmap *p)
{
	if (!xdr_ulong(x, &p->pm_prog))
		return FALSE;
	if (!xdr_ulong(x, &p->pm_vers))
		return FALSE;
	if (!xdr_ulong(x, &p->pm_prot))
		return FALSE;
	return xdr_ulong(x, &p->pm_port);
}
