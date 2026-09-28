/*
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 *
 * NFS version 3 (RFC 1813) support, derived from the NFS version 2
 * driver. See the file COPYING for copying and using conditions.
 */

# ifndef _sock_ipc_h
# define _sock_ipc_h

# include "global.h"


/* this structure is used to hold requests in a linked list */
typedef struct message MESSAGE;
struct message
{
	/* the public members */
	long	buf_len;	/* total length of the buffer */
	void	*buffer;	/* pointer to original memory block, if 0L dont free */
	long	data_len;	/* length of the data to send */
	char	*data;		/* data to send */
	long	hdr_len;	/* length of header buffer */
	char	*header;	/* the header data in xdr format */
	long	flags;		/* flags for freeing this structure after use */
# define FREE_BUFFER	0x80000000
# define FREE_DATA	0x40000000
# define FREE_HEADER	0x20000000
# define FREE_MSG	0x08000000
# define FROM_LIST	0x01000000
# define DATA_FLAGS	(FREE_BUFFER|FREE_DATA|FREE_HEADER)

	/* Credentials to send this request with, NULL for those of the
	 * calling process.
	 */
	NFS_CRED *cred;

	/* This is used only internally */
	MESSAGE	*next;		/* internal link */
	ulong	xid;		/* transaction id */
};

void		free_message (MESSAGE *m);
MESSAGE *	alloc_message (MESSAGE *m, char *buf, long buf_len, long data_size);

long	rpc_request (SERVER_OPT *opt, MESSAGE *mreq, ulong proc, MESSAGE **mrep);
int	init_ipc (ulong prog, ulong version);

/* Remember the calling process' credentials for later use */
void	nfs_capture_cred (NFS_CRED *c);


# endif /* _sock_ipc_h */
