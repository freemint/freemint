/*
 * Copyright 1993, 1994 by Ulrich KÅhn. All rights reserved.
 *
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 */

/*
 * File : common.h
 *        common declarations
 */

# ifndef _nfsmnt_h
# define _nfsmnt_h


extern long retrycnt;

extern long rsize;
extern long wsize;
extern long timeo;
extern long retrans;
extern int port;
extern int soft;
extern int intr;
extern int secure;
extern long actimeo;
extern int noac;



long do_nfs_mount(const char *remote, const char *local);
long do_nfs_unmount(const char *remote, const char *local);


extern char *commandname;

# endif
