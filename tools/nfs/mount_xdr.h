/*
 * Copyright 1993, 1994 by Ulrich KÅhn. All rights reserved.
 *
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 */

/*
 * File : mount_xdr.h
 *        specification of the mount protocol
 */

# ifndef _mount_xdr_h
# define _mount_xdr_h

# include <sys/types.h>
# include <rpc/xdr.h>


/* request numbers for nfs mount service */
#define MOUNTPROC_NULL     0
#define MOUNTPROC_MNT      1
#define MOUNTPROC_DUMP     2
#define MOUNTPROC_UMNT     3
#define MOUNTPROC_UMNTALL  4
#define MOUNTPROC_EXPORT   5

#define MOUNT_PROGRAM   100005
#define MOUNT_VERSION   1
#define MOUNT_MAXPROC   5


#define MNTPATHLEN   1024
#define MNTNAMLEN     255
#define MNTFHSIZE      32


bool_t xdr_dirpath (XDR *x, char *s);
bool_t xdr_name (XDR *x, char *s);


typedef struct fhandle
{
	char data[MNTFHSIZE];
} fhandle;

extern bool_t xdr_fhandle (XDR *x, fhandle *fhp);
#define xdr_size_fhandle(fhp)  (sizeof(fhandle))


typedef struct fhstatus
{
	u_long status;
	union
	{
		fhandle directory;   /* if status == 0 */
	} fhstatus_u;
} fhstatus;

bool_t xdr_fhstatus (XDR *x, fhstatus *fhsp);
long xdr_size_fhstatus (fhstatus *fhsp);


typedef struct mountlist
{
	char *ml_hostname;
	char *ml_directory;
	struct mountlist *ml_next;
} mountlist;

bool_t xdr_mountlist (XDR *x, mountlist *mlp);
long xdr_size_mountlist (mountlist *mlp);


typedef struct groups
{
	char *gr_name;
	struct groups *gr_next;
} groups;

bool_t xdr_groups (XDR *x, groups *gp);
long xdr_size_groups (groups *gp);


typedef struct exportlist
{
	char *ex_filesys;
	groups *ex_groups;
	struct exportlist *ex_next;
} exportlist;

bool_t xdr_exportlist (XDR *x, exportlist *elp);
long xdr_size_exportlist (exportlist *elp);


# endif
