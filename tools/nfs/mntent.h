/*
 * Copyright 1993, 1994 by Ulrich KÅhn. All rights reserved.
 *
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 */

# ifndef _mntent_h
# define _mntent_h

# include <stdio.h>


# define MNTTAB			"/etc/fstab"
# define MOUNTED		"/etc/mtab"

# define MNTMAXSTR		64

/* file system types */
# define MNTTYPE_42		"4.2"		/* 4.2 file system */
# define MNTTYPE_NFS		"nfs"		/* network fs */
# define MNTTYPE_PC		"pc"		/* DOS (FAT) fs */
# define MNTTYPE_SWAP		"swap"		/* swap fs */
# define MNTTYPE_IGNORE		"ignore"	/* ignore this entry */
# define MNTTYPE_LO		"lo"		/* loop back fs */
# define MNTTYPE_MNX		"minix"		/* minix fs */

/* mount options */
# define MNTOPT_RO		"ro"		/* read only */
# define MNTOPT_RW		"rw"		/* read/write */
# define MNTOPT_GRPID		"grpid"		/* SysV compatible group-id on create */
# define MNTOPT_REMOUNT		"remount"	/* change options for previosus mount */
# define MNTOPT_NOAUTO		"noauto"	/* hide entry for mount -a */
# define MNTOPT_NOSUB		"nosub"		/* disallow mounts beneath this mount */

/* 4.2 specific options */
# define MNTOPT_QUOTA		"quota"		/* with quota */
# define MNTOPT_NOQUOTA		"noquota"	/* w/o quota */

/* NFS specific options */
# define MNTOPT_SOFT		"soft"		/* soft mount */
# define MNTOPT_HARD		"hard"		/* hard mount (default) */
# define MNTOPT_NOSUID		"nosuid"	/* no set uid allowed */
# define MNTOPT_INTR		"intr"		/* allow interrupts on hard mount */
# define MNTOPT_SECURE		"secure"	/* use secure RPC */
# define MNTOPT_NOAC		"noac"		/* do not cache file attributes */
# define MNTOPT_NOCTO		"nocto"		/* no "close to open" attr consistency */
# define MNTOPT_PORT		"port"		/* server port no */
# define MNTOPT_RETRANS		"retrans"	/* number of request retries */
# define MNTOPT_RSIZE		"rsize"		/* read size (bytes) */
# define MNTOPT_WSIZE		"wsize"		/* write size (bytes) */
# define MNTOPT_TIMEO		"timeo"		/* initial timeout (1/10 sec) */
# define MNTOPT_ACTIMEO		"actimeo"	/* attr cache timeout (sec) */
# define MNTOPT_ACREGMIN	"acregmin"	/* min ac timeout for reg files (sec) */
# define MNTOPT_ACREGMAX	"acregmax"	/* max ac timeout for reg files */
# define MNTOPT_ACDIRMIN	"acdirmin"	/* min ac timeout for dirs */
# define MNTOPT_ACDIRMAX	"acdirmax"	/* max ac timeout for dirs */
# define MNTOPT_POSIX		"posix"		/* ask for static pathconf values */
                                 		/* from mountd */

/* Information about the mount entry */
# define MNTINFO_DEV		"dev"		/* device number of the mounted fs */

struct mntent
{
	char *mnt_fsname;	/* name of mounted file system */
	char *mnt_dir;		/* file system path prefix */
	char *mnt_type;		/* MNTTYPE_* */
	char *mnt_opts;		/* MNTOPT_* */
	int  mnt_freq;		/* dump frequenciy in days */
	int  mnt_passno;	/* pass number on parallel fsck */
};


FILE *		setmntent (char *name, char *type);
struct mntent *	getmntent (FILE *fp);
int		addmntent (FILE *fp, struct mntent *mnt);
char *		hasmntopt (struct mntent *mnt, char *opt);
int		endmntent (FILE *fp);


# endif
