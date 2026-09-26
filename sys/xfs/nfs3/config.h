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
 * config.h  means of configuration for the NFS3 XFS
 */

#ifndef CONFIG_H
#define CONFIG_H


#define CLUSTER_SIZE   64      /* number of file indices per cluster */
#define MAX_CLUSTER     8      /* number of clusters */

#define MAX_LABEL    63

/* mount point of this file system below u:\ */
#define NFS3_MOUNTPOINT   "u:\\nfs3"

/* default server port */
#define DEFAULT_PORT   2049


/* 200 Hz ticks before invalidating xattr struct in index structure */
#define DEFAULT_ACTIMEO  6000   /* 30 seconds */


#define DEFAULT_RSIZE 4096
#define DEFAULT_WSIZE 4096


/* To speed up buffer allocation, some space on the stack is used. These
 * constants control their size. Make sure that MiNT's system stack
 * is not overloaded! At the moment (MiNT 1.19) it is 8kb big.
 * If a request does not fit, alloc_message() falls back to kmalloc(),
 * so these are pure optimisations and may be kept small.
 *
 * Compared to the NFS2 driver the buffers had to grow: an nfs_fh3 is up
 * to 64+4 bytes instead of a fixed 32, and sattr3 adds a discriminator
 * word in front of every attribute.
 */
#define LOOKUPBUFSIZE      384   /* size of the buffer on the stack */
#define CREATEBUFSIZE      448
#define XATTRBUFSIZE        96
#define SATTRBUFSIZE       192
#define REMBUFSIZE         384
#define RENBUFSIZE         128
#define DFREEBUFSIZE        96
#define SYMLNBUFSIZE       128
#define READLNBUFSIZE       96
#define HARDLNBUFSIZE      448
#define READBUFSIZE         96
#define READDIRBUFSIZE     128
#define FSINFOBUFSIZE       96
#define COMMITBUFSIZE       96

#define MAX_RPC_HDR_SIZE  1024   /* xid, prog/vers/proc and AUTH_UNIX */

/* Largest RPC record we accept over TCP: a READ reply with a full rsize
 * of data plus its headers. Used as the reassembly buffer per connection.
 */
#define MAX_TCP_RECORD    (MAXDATA + 1024)

/* maximum number of bytes we ask for in a single nfs_readdir request */
#define MAX_READDIR_LEN    4096



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
