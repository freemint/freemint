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

#ifndef MOUNT_H
#define MOUNT_H

#include <sys/types.h>
#include "xdr.h"


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


bool_t xdr_dirpath(xdrs *x, char *s);
bool_t xdr_name(xdrs *x, char *s);


typedef struct fhandle
{
	opaque data[MNTFHSIZE];
} fhandle;

extern bool_t xdr_fhandle(xdrs *x, fhandle *fhp);
#define xdr_size_fhandle(fhp)  (sizeof(fhandle))


typedef struct fhstatus
{
	u_long status;
	union
	{
		fhandle directory;   /* if status == 0 */
	} fhstatus_u;
} fhstatus;

bool_t xdr_fhstatus(xdrs *x, fhstatus *fhsp);
long xdr_size_fhstatus(fhstatus *fhsp);


typedef struct mountlist
{
	char *ml_hostname;
	char *ml_directory;
	struct mountlist *ml_next;
} mountlist;

bool_t xdr_mountlist(xdrs *x, mountlist *mlp);
long xdr_size_mountlist(mountlist *mlp);


typedef struct groups
{
	char *gr_name;
	struct groups *gr_next;
} groups;

bool_t xdr_groups(xdrs *x, groups *gp);
long xdr_size_groups(groups *gp);


typedef struct exportlist
{
	char *ex_filesys;
	groups *ex_groups;
	struct exportlist *ex_next;
} exportlist;

bool_t xdr_exportlist(xdrs *x, exportlist *elp);
long xdr_size_exportlist(exportlist *elp);

#endif
