/*
 *	This is a simple netstat(8). Currently the following options are
 *	implemented:
 *
 *	-f [inet|unix]	-- show inet/unix sockets
 *	-a		-- show all sockets, even if they are listening
 *			   TCP sockets which are not shown by default
 *
 *	(w) 1993,1994, Kay Roemer.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>

/* possible socket states */
enum so_state
{
	SS_VIRGIN = 0,
	SS_ISUNCONNECTED,
	SS_ISCONNECTING,
	SS_ISCONNECTED,
	SS_ISDISCONNECTING,
	SS_ISDISCONNECTED
};

/* possible socket flags */
# define SO_ACCEPTCON	0x0001		/* socket is accepting connections */
# define SO_RCVATMARK	0x0002		/* in-band and oob data are in sync */
# define SO_CANTRCVMORE	0x0004		/* shut down for receives */
# define SO_CANTSNDMORE	0x0008		/* shut down for sends */
# define SO_CLOSING	0x0010		/* socket is close()ing */
# define SO_DROP	0x0020		/* drop connecting socket when accept() */


#define UNIX_DEVICE	"/dev/unix"
#define INET_DEVICE	"/dev/inet"

struct unix_info
{
	short	proto;			/* protcol numer, always 0 */
	short	flags;			/* socket flags, SO_* */
	short	type;			/* socket type, SOCK_DGRAM or SOCK_STREAM */
	short	state;			/* socket state, SS_* */
	short	qlen;			/* bytes in read buffer */
	short	addrlen;		/* addrlen, 0 if no address */
	struct sockaddr_un addr;	/* addr, only meaningful if addrlen > 0 */
};

struct inet_info
{
	short	proto;			/* protocol, IPPROTO_* */
	long	sendq;			/* bytes in send queue */
	long	recvq;			/* bytes in recv queue */
	short	state;			/* state */
	struct sockaddr_in laddr;	/* local address */
	struct sockaddr_in faddr;	/* foreign address */
};


static short show_unix_opt = 0;
static short show_usage_opt = 0;
static short show_inet_opt = 1;
static short show_all_opt = 0;


/*
 *	UNIX domain stuff.
 */

static char *
un_decode_state (short state)
{
	switch (state)
	{
		case SS_VIRGIN:
			return "VIRGIN";

		case SS_ISUNCONNECTED:
			return "UNCONNECTED";

		case SS_ISCONNECTING:
			return "CONNECTING";

		case SS_ISCONNECTED:
			return "CONNECTED";

		case SS_ISDISCONNECTING:
			return "DISCONNECTING";

		case SS_ISDISCONNECTED:
			return "DISCONNECTED";
	}

	return "UNKNOWN";
}

static char *
un_decode_type (short type)
{
	switch (type)
	{
		case SOCK_DGRAM:
			return "DGRAM";

		case SOCK_STREAM:
			return "STREAM";

		case SOCK_RDM:
			return "RDM";

		case SOCK_SEQPACKET:
			return "SEQPACKET";

		case SOCK_RAW:
			return "RAW";
	}

	return "UNKNOWN";
}

static char *
un_decode_flags (short flags)
{
	static char strflags[20];

	strflags[0] = '\0';

	if (flags & SO_ACCEPTCON)
		strcat (strflags, "ACC ");
	if (flags & SO_CANTSNDMORE)
		strcat (strflags, "SND ");
	if (flags & SO_CANTRCVMORE)
		strcat (strflags, "RCV ");
	if (flags & SO_RCVATMARK)
		strcat (strflags, "MRK ");

	return strflags;
}

static void
show_unix (void)
{
	struct unix_info info;
	char *proto;
	int fd, r;

	fd = open (UNIX_DEVICE, O_RDONLY);
	if (fd < 0)
	{
		fprintf (stderr, "cannot open device %s: %s\n",
			UNIX_DEVICE, strerror (errno));
		return;
	}

	puts ("Unix domain communication endpoints:");
	puts ("Proto   Flags            Type      State         Recv-Q Address");
	for (;;)
	{
		r = read (fd, &info, sizeof (info));
		if (r < 0)
		{
			fprintf (stderr, "cannot read next entry from %s: %s\n",
						UNIX_DEVICE, strerror (errno));
			break;
		}

		if (!r || r != sizeof (info))
			break;

		switch (info.proto)
		{
		case 0:
			proto = "unix";
			break;

		default:
			proto = "unknown";
			break;
		}

		printf ("%-7s %-16s %-9s %-13s %6u %s\n",
			proto,
			un_decode_flags (info.flags),
			un_decode_type (info.type),
			un_decode_state (info.state),
			info.qlen,
			info.addrlen ? info.addr.sun_path : "");
	}

	close (fd);
}

/*
 * INET domain stuff.
 */

static char *
in_decode_state (short state)
{
	switch (state)
	{
		case TCPS_CLOSED:
			return "CLOSED";

		case TCPS_LISTEN:
			return "LISTEN";

		case TCPS_SYNSENT:
			return "SYNSENT";

		case TCPS_SYNRCVD:
			return "SYNRCVD";

		case TCPS_ESTABLISHED:
			return "ESTABLISHED";

		case TCPS_FINWAIT1:
			return "FINWAIT1";

		case TCPS_FINWAIT2:
			return "FINWAIT2";

		case TCPS_CLOSEWAIT:
			return "CLOSEWAIT";

		case TCPS_LASTACK:
			return "LASTACK";

		case TCPS_CLOSING:
			return "CLOSING";

		case TCPS_TIMEWAIT:
			return "TIMEWAIT";
	}

	return "UNKNOWN";
}

static char *
in_decode_addr (struct sockaddr_in *in, char *proto)
{
	char buf[40];
	struct servent *sent;

	if (in->sin_port == 0)
		return "*:*";

	sent = getservbyport (in->sin_port, proto);
	if (sent)
	{
		sprintf (buf, "%s:%s",
			in->sin_addr.s_addr == INADDR_ANY
				? "*" : inet_ntoa (in->sin_addr),
			sent->s_name);
	}
	else
	{
		sprintf (buf, "%s:%d",
			in->sin_addr.s_addr == INADDR_ANY
				? "*" : inet_ntoa (in->sin_addr),
			in->sin_port);
	}

	return strdup (buf);
}

static char *
in_decode_proto (short proto)
{
	switch (proto)
	{
		case IPPROTO_UDP:
			return "UDP";

		case IPPROTO_TCP:
			return "TCP";
	}

	return "UNKNOWN";
}

static void
show_udp (void)
{
	struct inet_info info;
	char *laddr, *faddr;
	int fd, r;

	fd = open (INET_DEVICE, O_RDONLY);
	if (fd < 0)
	{
		fprintf (stderr, "cannot open device %s: %s\n",
			INET_DEVICE, strerror (errno));
		return;
	}

	puts ("Active UDP connections");
	puts ("Proto  Recv-Q Send-Q Local-Address        "
	      "Foreign-Address      State");
	for (;;)
	{
		r = read (fd, &info, sizeof (info));
		if (r < 0)
		{
			fprintf (stderr, "cannot read next entry from %s: %s",
						INET_DEVICE, strerror (errno));
			break;
		}

		if (!r || r != sizeof (info))
			break;
		
		if (info.proto != IPPROTO_UDP)
			continue;

		laddr = in_decode_addr (&info.laddr, "udp");
		faddr = in_decode_addr (&info.faddr, "udp");

		printf ("%-6s %6lu %6lu %-20s %-20s %-15s\n",
			in_decode_proto (info.proto),
			info.recvq, info.sendq,
			laddr, faddr,
			in_decode_state (info.state));

		free (laddr);
		free (faddr);
	}

	close (fd);
}

static void
show_tcp (void)
{
	struct inet_info info;
	char *laddr, *faddr;
	int fd, r;

	fd = open (INET_DEVICE, O_RDONLY);
	if (fd < 0)
	{
		fprintf (stderr, "cannot open device %s: %s\n",
			INET_DEVICE, strerror (errno));
		return;
	}

	puts ("Active TCP connections");
	puts ("Proto  Recv-Q Send-Q Local-Address        "
	      "Foreign-Address      State");
	for (;;)
	{
		r = read (fd, &info, sizeof (info));
		if (r < 0) {
			fprintf (stderr, "cannot read next entry from %s: %s",
						INET_DEVICE, strerror (errno));
			break;
		}

		if (!r || r != sizeof (info))
			break;

		if (info.proto != IPPROTO_TCP)
			continue;

		if (!show_all_opt && info.state == TCPS_LISTEN)
			continue;


		laddr = in_decode_addr (&info.laddr, "tcp");
		faddr = in_decode_addr (&info.faddr, "tcp");

		printf ("%-6s %6lu %6lu %-20s %-20s %-15s\n",
			in_decode_proto (info.proto),
			info.recvq, info.sendq,
			laddr, faddr,
			in_decode_state (info.state));

		free (laddr);
		free (faddr);
	}

	close (fd);
}

static void
usage (void)
{
	puts ("Usage: netstat [options]");
	puts ("Options:");
	puts ("\t [-a] [-f [unix|inet]]");
}

int
main (int argc, char *argv[])
{
	int c;

	while ((c = getopt (argc, argv, "af:")) != EOF)
	{
		switch (c)
		{
			case 'f':
				if (!strcmp (optarg, "unix"))
					show_unix_opt = 1;
				else if (!strcmp (optarg, "inet"))
					show_inet_opt = 1;
				else
					show_usage_opt = 1;
				break;

			case 'a':
				show_all_opt = 1;
				break;

			case '?':
				show_usage_opt = 1;
				break;
		}
	}

	if (show_usage_opt)
	{
		usage ();
		return 0;
	}

	if (show_inet_opt)
	{
		show_tcp ();
		show_udp ();
	}

	if (show_unix_opt)	
		show_unix ();

	return 0;
}
