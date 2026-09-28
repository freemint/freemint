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
 * File : global.c
 *        the few variables that are shared across the whole driver
 */

# include "global.h"


/* bytes currently allocated through own_kmalloc(), for leak hunting */
ulong memory = 0;

/* set when the kernel supports native UTC time stamps */
ushort native_utc = 0;

/* the device number we have to deal with */
int nfs_dev;
