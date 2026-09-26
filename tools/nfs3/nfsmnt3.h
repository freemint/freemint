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
 * File : nfsmnt3.h
 *        common declarations
 */

# ifndef _nfsmnt3_h
# define _nfsmnt3_h


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
/* -1 = try TCP and fall back to UDP, 0 = UDP only, 1 = TCP only */
extern int transport;


long do_nfs_mount (const char *remote, const char *local);
long do_nfs_unmount (const char *remote, const char *local);


extern char *commandname;

/* name this file system goes under in \etc\mtab */
extern const char *fstype;

# endif
