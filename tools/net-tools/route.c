/*
 *	route(8) utility for MintNet, (w) 1994, Kay Roemer.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/route.h>
#include <netdb.h>
#include <sockios.h>

#define SIN(x)			((struct sockaddr_in *)x)

#define _PATH_DEV_ROUTE		"/dev/route"


/* structure obtained from /dev/route */
struct route_info
{
	char nif[IFNAMSIZ];
	struct rtentry	rt;
};

static int sock;

static struct in_addr
resolve (char *name)
{
	struct hostent *hent;
	struct in_addr ina = { INADDR_ANY };

	if (stricmp (name, "default"))
	{
		hent = gethostbyname (name);
		if (!hent)
		{
			fprintf (stderr, "cannot lookup hostname %s", name);
			herror ("");
			exit (1);
		}

		ina = *(struct in_addr *) hent->h_addr;
	}

	return ina;
}

static int
is_host (struct in_addr ina)
{
	return (ina.s_addr != INADDR_ANY && inet_lnaof (ina) != 0);
}

static void
usage (void)
{
	printf ("route\t [del <target>]\n");
	printf ("\t [add <target> <interface> [gw <gateway>] [metric NN]]\n");
	exit (0);
}

static void
add_route (int argc, char *argv[])
{
#define NEXTARG(i) { if (++i >= argc) usage (); }
	struct rtentry rt;
	int i = 1;

	if (argc <= 1)
		usage ();

	memset (&rt, 0, sizeof (rt));
	SIN (&rt.rt_dst)->sin_family = AF_INET;
	SIN (&rt.rt_gateway)->sin_family = AF_INET;
	rt.rt_flags = RTF_UP;

	SIN (&rt.rt_dst)->sin_addr = resolve (argv[i]);
	if (is_host (SIN (&rt.rt_dst)->sin_addr))
		rt.rt_flags |= RTF_HOST;
	
	++i; /* skip interface name, we do not need it */
	while (++i < argc)
	{
		if (!strcmp (argv[i], "gw"))
		{
			NEXTARG (i);
			rt.rt_flags |= RTF_GATEWAY;
			SIN (&rt.rt_gateway)->sin_addr = resolve (argv[i]);
			if (!is_host (SIN (&rt.rt_gateway)->sin_addr))
			{
				printf ("GATEWAY cannot be a NETWORK\n");
				exit (1);
			}
		}
		else if (!strcmp (argv[i], "metric"))
		{
			NEXTARG (i);
			rt.rt_metric = atol (argv[i]);
		} 
		else
			usage ();
	}

	if (ioctl (sock, SIOCADDRT, &rt) < 0)
	{
		perror ("cannot add route");
		exit (1);
	}
}

static void
del_route (int argc, char *argv[])
{
	struct rtentry rt;

	if (argc <= 1)
		usage ();

	memset (&rt, 0, sizeof (rt));
	SIN (&rt.rt_dst)->sin_family = AF_INET;
	SIN (&rt.rt_dst)->sin_addr = resolve (argv[1]);
	if (is_host (SIN (&rt.rt_dst)->sin_addr))
		rt.rt_flags |= RTF_HOST;

	if (ioctl (sock, SIOCDELRT, &rt) < 0)
	{
		perror ("cannot delete route");
		exit (1);
	}
}

static char *
decode_flags (short flags)
{
	static char str[16];

	str[0] = '\0';

	if (flags & RTF_UP)
		strcat (str, "U");
	if (flags & RTF_HOST)
		strcat (str, "H");
	if (flags & RTF_GATEWAY)
		strcat (str, "G");
	if (flags & RTF_REJECT)
		strcat (str, "R");
	if (!(flags & RTF_STATIC))
		strcat (str, "D");

	return str;
}

static void
show_routes (void)
{
	struct route_info rtinfo;
	struct in_addr net, gway;
	char *sflags, *sgway, *snet;
	short flags;
	int fd, r;

	fd = open (_PATH_DEV_ROUTE, O_RDONLY);
	if (fd < 0)
	{
		fprintf (stderr, "cannot open device %s: %s\n",
			_PATH_DEV_ROUTE,
			strerror (errno));
		exit (1);
	}

	printf ("Destination         Gateway             "
		"Flags   Ref      Use Metric Iface\n");

	while (1)
	{
		r = read (fd, &rtinfo, sizeof (rtinfo));
		if (r < 0)
		{
			fprintf (stderr, "cannot read next routing table "
				"entry from %s: %s\n",
				_PATH_DEV_ROUTE,
				strerror (errno));
			exit (1);
		}
		else if (r != sizeof (rtinfo))
			break;

		flags = rtinfo.rt.rt_flags;
		sflags = decode_flags (flags);
		net = SIN (&rtinfo.rt.rt_dst)->sin_addr;
		gway = SIN (&rtinfo.rt.rt_gateway)->sin_addr;

		snet = strdup (net.s_addr == INADDR_ANY
			? "default"
			: inet_ntoa (net));

		sgway = strdup (flags & RTF_GATEWAY
			? inet_ntoa (gway)
			: "*");

		printf ("%-20s%-20s%-6s%5d%9ld%7ld %-6s\n",
			snet, sgway, sflags,
			rtinfo.rt.rt_refcnt,
			rtinfo.rt.rt_use,
			rtinfo.rt.rt_metric,
			rtinfo.nif);

		free (snet);
		free (sgway);
	}

	close (fd);
}

int
main (int argc, char *argv[])
{
	sock = socket (PF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
	{
		perror ("cannot open socket");
		return 1;
	}

	if (argc <= 1)
		show_routes ();
	else if (!strcmp (argv[1], "add"))
		add_route (argc-1, &argv[1]);
	else if (!strcmp (argv[1], "del"))
		del_route (argc-1, &argv[1]);
	else
		usage ();

	return 0;
}
