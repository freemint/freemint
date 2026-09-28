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

/*
 * config.h  means of configuration for the networking XFS
 */

#ifndef CONFIG_H
#define CONFIG_H


#define CLUSTER_SIZE   64      /* number of file indices per cluster */
#define MAX_CLUSTER     8      /* number of clusters */

#define MAX_LABEL    63

#define NFS_DRV   ('w'-'a')

/* default server port */
#define DEFAULT_PORT   2049


/* 200 Hz ticks before invalidating xattr struct in index structure */
#define DEFAULT_ACTIMEO  6000   /* 30 seconds */


#define DEFAULT_RSIZE 4096 
#define DEFAULT_WSIZE 4096 



/* To speed up buffer allocation, some space on the stack is used. These
 * constants control their size. Make sure that MiNT's system stack
 * is not overloaded! At the moment (MiNT 1.09) it is 8kb big.
 * These values should be sufficient for most cases to avoid additional
 * memory allocation.
 */
#define LOOKUPBUFSIZE      128   /* size of the buffer on the stack */
#define CREATEBUFSIZE      128
#define XATTRBUFSIZE       128
#define SATTRBUFSIZE       128
#define REMBUFSIZE         128
#define RENBUFSIZE         128
#define DFREEBUFSIZE        64
#define SYMLNBUFSIZE       128
#define READLNBUFSIZE      128
#define HARDLNBUFSIZE      128
#define READBUFSIZE        128
#define READDIRBUFSIZE     128

#define MAX_RPC_HDR_SIZE   4096  /* FIXME: this value is too small */

/* maximum number of bytes in a reply for nfs_readdir */
#define MAX_READDIR_LEN    4108	/* value has been increased because of Ubuntu NFS server problems */



/* configuration values for the resend code */
#define DEFAULT_RETRANS  5 
#define DEFAULT_TIMEO    400      /* 2 sec in ticks */


/* config values for the lookup cache */
#define USE_CACHE         /* use the lookup cache */
#define LOOKUP_CACHE_SIZE  64
#define NFS_CACHE_EXPIRE  1000   /* 5 seconds */

/* when a process is running in TOS-Domain, convert filenames to
 * lower case before sending the request to the daemon, which might
 * be running in MiNT domain.
 */
#define TOSDOMAIN_LOWERCASE


#endif
