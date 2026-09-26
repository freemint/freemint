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
 * File : nfsmnt3.c
 *        do an NFS version 3 mount
 */

# include <errno.h>
# include <stdio.h>
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
# include "mount_xdr3.h"
# include "nfsmnt3.h"


const char *fstype = "nfs3";

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

/* Transport: -1 means try TCP and fall back to UDP, which is the
 * default because a current Linux nfsd serves TCP without any extra
 * configuration. 0 pins UDP, 1 pins TCP.
 */
int transport = -1;


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
#define OPT_TCP     0x0800
#define OPT_UDPFALL 0x1000


#define MOUNT_PORT  2050
#define NFS3_MOUNT_VERS 3


typedef struct myxattr MYXATTR;

/* structure for getxattr, must match the kernel's XATTR */
struct myxattr
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

/* Must match nfs_fh3 in the driver: a length word followed by up to 64
 * bytes of handle. Version 1 of this interface had a fixed 32 byte
 * array here, which is why the version field had to be bumped.
 */
typedef struct
{
	unsigned long	len;
	char		data[FHSIZE3];
} my_nfs_fh3;

typedef struct
{
	long		version;
	my_nfs_fh3	handle;	/* initial file handle from the server's mountd */
	MYXATTR		mntattr;	/* not used yet */
	long		flags;
	long		rsize;
	long		wsize;

	short		retrans;	/* `int' in the kernel, which is -mshort */
	long		timeo;
	long		actimeo;
	long		reserved[8];

	struct sockaddr_in server;
	char hostname[256];
} NFS_MOUNT_INFO;



# define NFS3_MOUNT	(('N' << 8) | 3)
# define NFS3_UNMOUNT	(('N' << 8) | 4)

/* only for debugging purposes */
# define NFS3_MNTDUMP	(('N' << 8) | 42)
# define NFS3_DUMPALL	(('N' << 8) | 43)


# define IPNAMELEN	256



static int
make_socket (void)
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

/* Split "host:/exported/path" into its two halves. */
static const char *
split_remote (const char *remote, char *hostname, size_t hostlen)
{
	const char *p = strchr (remote, ':');
	size_t n;

	if (!p)
	{
		hostname[0] = '\0';
		return remote;
	}

	n = (size_t) (p - remote);
	if (n >= hostlen)
		n = hostlen - 1;

	memcpy (hostname, remote, n);
	hostname[n] = '\0';

	return p + 1;
}

static CLIENT *
mount_client (struct sockaddr_in *server, int *s)
{
	struct timeval retry_time = { 1, 0 };  /* every second */
	CLIENT *cl;

	server->sin_port = htons (0);  /* ask the port mapper for the port */
	cl = clntudp_create (server, MOUNT_PROGRAM, MOUNT_V3, retry_time, s);
	if (!cl)
	{
		/* also try a fallback method with a fixed port number */
		server->sin_port = htons (MOUNT_PORT);
		cl = clntudp_create (server, MOUNT_PROGRAM, MOUNT_V3, retry_time, s);
	}

	return cl;
}

#pragma GCC diagnostic ignored "-Wcast-qual"

long
do_nfs_mount (const char *remote, const char *localdir)
{
	long r;
	NFS_MOUNT_INFO info;
	char mountname[MNTPATHLEN+1];
	char hostname[IPNAMELEN+1];
	struct timeval total_time = { 5, 0 };  /* total timeout */
	enum clnt_stat res;
	mountres3 mres;
	CLIENT *cl;
	int s;
	struct sockaddr_in server;
	struct hostent *hp;


	unx2dos (localdir, mountname);
	remote = split_remote (remote, hostname, sizeof (hostname));

	memset (&info, 0, sizeof (info));
	info.version = NFS3_MOUNT_VERS;
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
	if (transport != 0)
		info.flags |= OPT_TCP;
	if (transport < 0)
		info.flags |= OPT_UDPFALL;

	info.retrans = retrans;
	info.timeo = timeo * CLOCKS_PER_SEC/10;
	info.rsize = rsize;
	info.wsize = wsize;
	info.actimeo = actimeo * CLOCKS_PER_SEC/10;
	{
		size_t hl = strlen (hostname);

		if (hl > sizeof (info.hostname) - 1)
			hl = sizeof (info.hostname) - 1;

		memcpy (info.hostname, hostname, hl);
		info.hostname[hl] = '\0';
	}

	s = make_socket ();
	if (s < 0)
		return 1;

	/* get the server address from the net database */
	hp = gethostbyname (hostname);
	if (!hp)
	{
		fprintf (stderr, "%s: failed to look up server address\n", commandname);
		return 1;
	}

	if (hp->h_addrtype != AF_INET)
	{
		fprintf (stderr, "%s: not AF_INET address type\n", commandname);
		return 1;
	}

	server.sin_family = AF_INET;
	memcpy ((char *) &server.sin_addr, hp->h_addr, hp->h_length);

	cl = mount_client (&server, &s);
	if (!cl)
	{
		fprintf (stderr, "%s: failed to create RPC client for "
			 "MOUNT version 3\n", commandname);
		return 1;
	}

	memset (&mres, 0, sizeof (mres));
	res = clnt_call (cl, MOUNTPROC3_MNT,
	                (xdrproc_t) xdr_dirpath, (void *) remote,
	                (xdrproc_t) xdr_mountres3, (void *) &mres, total_time);

	if (res != RPC_SUCCESS)
	{
		clnt_perror (cl, commandname);
		clnt_destroy (cl);

		return res;
	}

	clnt_destroy (cl);

	if (mres.status != MNT3_OK)
	{
		fprintf (stderr, "%s: mount request failed: %s (%lu)\n",
			 commandname, mountstat3_str (mres.status),
			 (unsigned long) mres.status);
		return -1;
	}

	if (mres.fhandle.len == 0 || mres.fhandle.len > FHSIZE3)
	{
		fprintf (stderr, "%s: server returned a bogus file handle "
			 "of %lu bytes\n", commandname,
			 (unsigned long) mres.fhandle.len);
		return -1;
	}

	if (verbose)
	{
		unsigned int i;

		printf ("got a %lu byte file handle, auth flavors:",
			(unsigned long) mres.fhandle.len);
		for (i = 0; i < mres.nauth; i++)
			printf (" %d", mres.auth_flavors[i]);
		printf ("\n");
	}

	info.server.sin_family = AF_INET;
	memcpy ((char *) &info.server.sin_addr, hp->h_addr, hp->h_length);
	info.server.sin_port = htons (port);
	info.handle.len = mres.fhandle.len;
	memcpy (info.handle.data, mres.fhandle.data, mres.fhandle.len);

	r = Dcntl (NFS3_MOUNT, mountname, &info);
	if (r != 0)
		fprintf (stderr, "%s: mount request to kernel failed (%ld)\n",
			 commandname, r);

	return r;
}

long
do_nfs_unmount (const char *remote, const char *local)
{
	long r;
	char mountname[MNTPATHLEN+1];
	char hostname[IPNAMELEN+1];
	struct timeval total_time = { 5, 0 };  /* total timeout */
	CLIENT *cl;
	int s;
	struct sockaddr_in server;
	struct hostent *hp;


	if (!local)
		return -1;

	unx2dos (local, mountname);

	/* do unmount on the local kernel -- this is the part that matters */
	r = Dcntl (NFS3_UNMOUNT, mountname, 0);
	if (r != 0)
	{
		fprintf (stderr, "%s: unmount request to kernel failed (%ld)\n",
			 commandname, r);
		return r;
	}

	/* Telling the server is courtesy: it only drops the entry from the
	 * server's rmtab. Without a remote name we cannot, and that is no
	 * reason to fail.
	 */
	if (!remote || !strchr (remote, ':'))
	{
		if (verbose)
			printf ("%s: no server known for %s, "
				"skipping MOUNT3 UMNT\n", commandname, local);
		return 0;
	}

	remote = split_remote (remote, hostname, sizeof (hostname));

	/* no error checks here, as we should not fail the unmount if there was
	 * no contact with the nfs server.
	 */
	s = make_socket ();
	if (s < 0)
		return 0;

	/* get the server address from the net database */
	hp = gethostbyname (hostname);
	if (!hp)
	{
		fprintf (stderr, "%s: failed to look up server address\n", commandname);
		return 0;
	}

	if (hp->h_addrtype != AF_INET)
	{
		fprintf (stderr, "%s: not AF_INET address type\n", commandname);
		return 0;
	}

	server.sin_family = AF_INET;
	memcpy ((char *) &server.sin_addr, hp->h_addr, hp->h_length);

	cl = mount_client (&server, &s);
	if (!cl)
		return 0;

	(void) clnt_call (cl, MOUNTPROC3_UMNT,
	                  (xdrproc_t) xdr_dirpath, (caddr_t) remote,
	                  (xdrproc_t) xdr_void, (caddr_t) NULL, total_time);
	clnt_destroy (cl);

	return 0;
}
