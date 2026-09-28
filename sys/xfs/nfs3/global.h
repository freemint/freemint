/*
 * Copyright 1993, 1994 by Ulrich KÅhn. All rights reserved.
 *
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 * 
 * Modified for FreeMiNT CVS
 * by Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
 */

# ifndef _global_h
# define _global_h

# include "mint/mint.h"

# include "libkern/libkern.h"
# include "mint/file.h"
# include "mint/time.h"
# include "mint/stat.h"

# include "sockets/inet4/in.h"

/* own default header */
# include "config.h"
# include "nfs_xdr.h"


/* debug section
 */

# if 0			/* disable debug diagnostics */
# define NFS_DEBUG	1
# endif

# ifdef NFS_DEBUG

# define FORCE(x)	
# define ALERT(x)	KERNEL_ALERT x
# define DEBUG(x)	KERNEL_DEBUG x
# define TRACE(x)	KERNEL_TRACE x
# define ASSERT(x)	assert x

# else

# define FORCE(x)	
# define ALERT(x)	KERNEL_ALERT x
# define DEBUG(x)	
# define TRACE(x)	
# define ASSERT(x)	assert x

# endif


/* memory allocation
 * 
 * include statistic analysis to detect
 * memory leaks
 */

extern ulong memory;

INLINE void *
own_kmalloc (long size)
{
	ulong *tmp;
	
	size += sizeof (*tmp);
	
	tmp = kmalloc (size);
	if (tmp)
	{
		*tmp++ = size;
		memory += size;
	}
	
	return tmp;
}

INLINE void
own_kfree (void *dst)
{
	ulong *tmp = dst;
	
	tmp--;
	memory -= *tmp;
	
	kfree (tmp);
}

# undef kmalloc
# undef kfree

# define kmalloc	own_kmalloc
# define kfree		own_kfree


/* global specials
 */

extern ushort native_utc;

INLINE long
current_time (void)
{
	if (native_utc)
		return utc.tv_sec;
	
	return unixtime (timestamp, datestamp);
}
# define CURRENT_TIME	current_time ()


/* these error numbers may only be used internally as they are not part
 * of the rpc definition
 */
# define ERPC_OK              0  /* not really an error */
# define ERPC_PROGUNAVAIL  -100  /* specified program does not exist */
# define ERPC_PROGMISMATCH -101  /* wrong program version number */
# define ERPC_PROCUNAVAIL  -102  /* specified procedure not supported */
# define ERPC_GARBAGEARGS  -103  /* bad arguments */
# define ERPC_RPCMISMATCH  -104  /* wrong rpc version */
# define ERPC_AUTHERROR    -105  /* authentification error */



# define OPT_DEFAULT		0x0000   /* not really an option */

# define OPT_RO			0x0001
# define OPT_GRPID		0x0002

# define OPT_SOFT		0x0010
# define OPT_NOSUID		0x0020
# define OPT_INTR		0x0040
# define OPT_SECURE		0x0080
# define OPT_NOAC		0x0100
# define OPT_NOCTO		0x0200
# define OPT_POSIX		0x0400

# define OPT_USE_DEFAULTS	0x8000   /* use defaults for timeout, port etc */
# define SERVER_OPTS		(OPT_SOFT | OPT_INTR)

typedef struct
{
	long flags;   /* only the server specific options */
	struct sockaddr_in addr;    /* the address of the server */
	int retrans;  /* number of request retries */
	long timeo;   /* initial timeout in 1/200 sec */
	long reserved[4];
	char hostname[256];
} SERVER_OPT;


typedef struct nfs_mount_opt NFS_MOUNT_OPT;
struct nfs_mount_opt
{
	NFS_MOUNT_OPT *next;
	long	flags;		/* the OPT_* values */
	SERVER_OPT server;
	long	rsize;
	long	wsize;
	long	actimeo;	/* attr cache timeout */
	long	res[8];
};


typedef struct index_cluster INDEX_CLUSTER;
typedef struct nfs_index NFS_INDEX;

struct nfs_index
{
	char	*name;		/* the name of the thing */
	long	search_val;	/* for faster search */
	long	flags;
# define IS_MOUNT_DIR	0x8000	/* this is a mounted directory */
# define NO_HANDLE	0x4000	/* we have no handle (this is set by */
				/* nfs_readdir, as the remote procedure */
				/* does not provide a handle */
	
	NFS_MOUNT_OPT *opt;	/* options for this mount and subdirs */
	INDEX_CLUSTER *cluster;	/* cluster this is in */
	nfs_fh	handle;		/* file handle for this on the server */
	long	link;		/* no of times this cookie is in use */
	XATTR	attr;
	long	stamp;		/* time stamp when this xattr struct was filled */
	struct nfs_index *dir;	/* index of directory this one is in */
	struct nfs_index *aux;	/* this is used for getname() */
	struct nfs_index *next;	/* only if too much of these are in use */
};

struct index_cluster
{
	int	cl_no;		/* this has this cluster number */
	int	n_used;		/* number of indices used in this cluster */
	INDEX_CLUSTER *next;	/* only if too much of these are used */
	NFS_INDEX index[CLUSTER_SIZE];
};


extern NFS_INDEX *mounted;
extern INDEX_CLUSTER *cluster[MAX_CLUSTER];
extern NFS_MOUNT_OPT *opt_list;


# define NFS_MOUNT_VERS  1

typedef struct
{
	long	version;	/* version of this structure, currently 1 */
	nfs_fh	handle;		/* initial file handle from the server's mountd */
	XATTR	mntattr;	/* not used yet */
	long	flags;		/* same as NFS_MOUNT_OPT.flags */
	long	rsize;
	long	wsize;
	
	int	retrans;	/* number of request retries */
	long	timeo;		/* initial timeout in 1/200 sec */
	long	actimeo;	/* attr cache timeout */
	long	reserved[8];	/* for future enhancements */
	
	struct sockaddr_in server;	/* address of the server */
	char hostname[256];
} NFS_MOUNT_INFO;


# define NFS_MOUNT	(('N'<< 8) | 1)
# define NFS_UNMOUNT	(('N'<< 8) | 2)

# define NFS_MNTDUMP	(('N'<< 8) | 42)
# define NFS_DUMPALL	(('N'<< 8) | 43)


/* the device number we have to deal with
 */
extern int nfs_dev;

# define ROOT_INDEX	NULL


# endif /* _global_h */
