/*
 * Copyright 1993, 1994 by Ulrich KÅhn. All rights reserved.
 *
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 */

/*
 * File : mount.h
 *        specification of the mount protocol
 */

# ifndef _mount_h
# define _mount_h

# include <sys/types.h>
# include <rpc/xdr.h>


/* option indicators */
extern int verbose;
extern int readonly;
extern int nosuid;
extern int grpid;


# endif
