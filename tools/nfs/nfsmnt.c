/*
 * Copyright 1993, 1994 by Ulrich KÅhn. All rights reserved.
 *
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 */

/*
 * File : nfsmnt.c
 *        do an nfs mount
 */

# include <errno.h>
# include <string.h>
# include <support.h>
# include <time.h>
# include <sys/socket.h>
# include <sys/stat.h>
# include <sys/types.h>

# include <netinet/in.h>
# include <netdb.h>
# include <rpc/clnt.h>

# include <osbind.h>
# include <mintbind.h>

# include "mount.h"
# include "mount_xdr.h"
# include "nfsmnt.h"


/* nfs specific option values */
long rsize = 0;
long wsize = 0;
long timeo = 0;
long retrans = 0;
long actimeo = 0;

int port = 0; /* use the default port as default */

int soft = 0;
int intr = 0;
int secure = 0;
int noac = 0;
int nosuid = 0;


#define OPT_DEFAULT 0x0000

#define OPT_RO      0x0001
#define OPT_GRPID   0x0002

#define OPT_SOFT    0x0010
#define OPT_NOSUID  0x0020
#define OPT_INTR    0x0040
#define OPT_SECURE  0x0080
#define OPT_NOAC    0x0100
#define OPT_NOCTO   0x0200
#define OPT_POSIX   0x0400


#define MOUNT_PORT  2050
#define NFS_MOUNT_VERS 1


typedef struct xattr XATTR;

/* structure for getxattr */
struct xattr
{
	ushort	mode;
	long	index;
	ushort	dev;
	ushort	rdev;		/* "real" device */
	ushort	nlink;
	ushort	uid;
	ushort	gid;
	long	size;
	long	blksize;
	long	nblocks;
	ushort	mtime, mdate;
	ushort	atime, adate;
	ushort	ctime, cdate;
	short	attr;
	short	reserved2;
	long	reserved3[2];
};

typedef struct
{
	long	version;
	fhandle	handle;		/* initial file handle from the server's mountd */
	XATTR	mntattr;	/* not used yet */
	long	flags;
	long	rsize;
	long	wsize;

	short	retrans;
	long	timeo;
	long	actimeo;
	long	reserved[8];

	struct sockaddr_in server;
	char hostname[256];
} NFS_MOUNT_INFO;



# define NFS_MOUNT	(('N' << 8) | 1)
# define NFS_UNMOUNT	(('N' << 8) | 2)

/* only for debugging purposes */
# define NFS_MNTDUMP	(('N' << 8) | 42)
# define NFS_DUMPALL	(('N' << 8) | 43)


# define IPNAMELEN	256
# define MOUNT_PORT	2050



# if 0
/* bindresvport() copied from nfs-050/xfs/sock_ipc.c  --cpbs */

long
my_bindresvport (int s)
{
	struct sockaddr_in sin;
	short port;
	long r;
	
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl (INADDR_ANY);
	
	for (port = IPPORT_RESERVED - 1; port > IPPORT_RESERVED/2; --port)
	{
		sin.sin_port = htons(port);
		r = bind(s, (struct sockaddr *) &sin, sizeof (sin));
		if (r == 0)
			return 0;

		if (r < 0 && errno != EADDRINUSE)
			return r;
	}
	
	return EADDRINUSE;
}
# endif


static int
make_socket (long maxmsgsize)
{
	struct sockaddr_in in;
	long res;
	int fd;
	
	fd = socket (PF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
	{
		fprintf (stderr, "open_connection: socket() failed with %d\n", errno);
		return fd;
	}
	
#if 0
	res = setsockopt (fd, SOL_SOCKET, SO_RCVBUF, &maxmsgsize, sizeof (long));
	if (res < 0)
	{
		fprintf (stderr, "open_connection: setsockopt() failed with %d\n", errno);
		return res;
	}
#endif
	
	in.sin_family = AF_INET;
	in.sin_addr.s_addr = htonl (INADDR_ANY);
	
	res = bindresvport (fd, &in);
	if (res < 0)
	{
		fprintf (stderr, "open_connection: bind() failed with %d\n", errno);
		return res;
	}
	
	return fd;
}

long
do_nfs_mount (char *remote, char *localdir)
{
	long r;
	NFS_MOUNT_INFO info;
	char mountname[MNTPATHLEN+1];
	char hostname[IPNAMELEN+1];
	struct timeval retry_time = { 1, 0 };  /* every second */
	struct timeval total_time = { 5, 0 };  /* total timeout */
	long maxmsgsize = MNTPATHLEN+RPCSMALLMSGSIZE;
	enum clnt_stat res;
	fhstatus fh;
	char *p;
	CLIENT *cl;
	int s;
	struct sockaddr_in server;
	struct hostent *hp;
	
	
	unx2dos (localdir, mountname);
	p = strchr (remote, ':');    /* find end of hostname */
	if (!p)
		strcpy (hostname, "");
	else
	{
		char c = *p;
		*p = '\0';
		strcpy (hostname, remote);
		*p = c;
		remote = p+1;
	}
	
	info.version = NFS_MOUNT_VERS;
	info.flags = OPT_DEFAULT;
	
	if (readonly)
		info.flags |= OPT_RO;
	if (nosuid)
		info.flags |= OPT_NOSUID;
	if (noac)
		info.flags |= OPT_NOAC;
	if (soft)
		info.flags |= OPT_SOFT;
	if (intr)
		info.flags |= OPT_INTR;
	if (secure)
		info.flags |= OPT_SECURE;
	
	info.retrans = retrans;
	info.timeo = timeo * CLOCKS_PER_SEC/10;
	info.rsize = rsize;
	info.wsize = wsize;
	info.actimeo = actimeo * CLOCKS_PER_SEC/10;
	strncpy (info.hostname, hostname, sizeof (info.hostname) - 1);
	info.hostname[sizeof(info.hostname)-1] = '\0';
	
	s = make_socket (maxmsgsize);
	if (s < 0)
		return 1;
	
	/* get the server address from the net database */
	hp = gethostbyname (hostname);
	if (!hp)
	{
		fprintf (stderr, "do_nfs_mount: failed to look up server address\n");
		return 1;
	}
	
	if (hp->h_addrtype != AF_INET)
	{
		fprintf(stderr, "do_nfs_mount: not AF_INET address type\n");
		return 1;
	}
	
	server.sin_family = AF_INET;
	memcpy ((char*) &server.sin_addr, hp->h_addr, hp->h_length);
	server.sin_port = htons (0);  /* ask the port mapper for that port */
	
	cl = clntudp_create (&server, MOUNT_PROGRAM, MOUNT_VERSION, retry_time, &s);
	if (!cl)
	{
		/* also try a fallback method with a fixed port number */
		server.sin_port = htons(MOUNT_PORT);
		cl = clntudp_create (&server, MOUNT_PROGRAM, MOUNT_VERSION, retry_time, &s);
		if (!cl)
		{
			fprintf (stderr, "do_nfs_mount: failed to create RPC client\n");
			return 1;
		}
	}
	
	res = clnt_call (cl, MOUNTPROC_MNT,
	                (xdrproc_t) xdr_dirpath, (caddr_t) remote,
	                (xdrproc_t) xdr_fhstatus, (caddr_t) &fh, total_time);
	
	if (res != RPC_SUCCESS)
	{
		clnt_perror (cl, "do_nfs_mount");
		clnt_destroy (cl);
		
		return res;
	}
	
	clnt_destroy (cl);
	
	if (fh.status != 0)
	{
		fprintf (stderr, "do_nfs_mount: mount request failed with %ld\n", fh.status);
		return -1;
	}
	
	info.server.sin_family = AF_INET;
	memcpy ((char*) &info.server.sin_addr, hp->h_addr, hp->h_length);
	info.server.sin_port = htons (port);
	info.handle = fh.fhstatus_u.directory;
	
	r = Dcntl (NFS_MOUNT, mountname, &info);
	if (r != 0)
		fprintf (stderr, "%s: mount request to kernel failed\n", commandname);
	
	return r;
}

long
do_nfs_unmount (char *remote, char *local)
{
	long r;
	char mountname[MNTPATHLEN+1];
	char hostname[IPNAMELEN+1];
	struct timeval retry_time = { 1, 0 };  /* every second */
	struct timeval total_time = { 5, 0 };  /* total timeout */
	long maxmsgsize = MNTPATHLEN+RPCSMALLMSGSIZE;
	enum clnt_stat res;
	CLIENT *cl;
	int s;
	char *p;
	struct sockaddr_in server;
	struct hostent *hp;
	
	
	if (!local)
		return -1;
	
	unx2dos (local, mountname);
	
	p = strchr (remote, ':');    /* find end of hostname */
	if (!p)
		strcpy(hostname, "");
	else
	{
		char c = *p;
		*p = '\0';
		strcpy (hostname, remote);
		*p = c;
		remote = p+1;
	}
	
	/* do unmount on the local kernel */
	r = Dcntl (NFS_UNMOUNT, mountname, 0);
	if (r != 0)
	{
		fprintf (stderr, "%s: unmount request to kernel failed\n", commandname);
		return r;
	}
	
	/* no error checks here, as we should not fail the unmount if there was
	 * no contact with the nfs server.
	 */
	s = make_socket (maxmsgsize);
	if (s < 0)
		return 0;
	
	/* get the server address from the net database */
	hp = gethostbyname (hostname);
	if (!hp)
	{
		fprintf(stderr, "do_nfs_mount: failed to look up server address\n");
		return 1;
	}
	
	if (hp->h_addrtype != AF_INET)
	{
		fprintf(stderr, "do_nfs_mount: not AF_INET address type\n");
		return 1;
	}
	
	server.sin_family = AF_INET;
	memcpy ((char*) &server.sin_addr, hp->h_addr, hp->h_length);
	server.sin_port = htons (0);  /* ask the port mapper for the port */
	
	cl = clntudp_create (&server, MOUNT_PROGRAM, MOUNT_VERSION, retry_time, &s);
	if (!cl)
	{
		/* Also try a fallback method with a fixed port number */
		server.sin_port = htons (MOUNT_PORT);
		cl = clntudp_create (&server, MOUNT_PROGRAM, MOUNT_VERSION, retry_time, &s);
		if (!cl)
			return 0;
	}
	
	res = clnt_call (cl, MOUNTPROC_UMNT,
	                  (xdrproc_t)xdr_dirpath, (caddr_t)remote,
	                  (xdrproc_t)xdr_void, (caddr_t)NULL, total_time);
	clnt_destroy (cl);
	return 0;
}
