/*
 *	ifconfig(8) utility for MintNet, (w) 1994, Kay Roemer.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#include <sockios.h>
#include "ifopts.h"


int sock;

static void
get_stats (char *name, struct ifstat *stats)
{
	struct ifreq ifr;

	strcpy (ifr.ifr_name, name);
	if (ioctl (sock, SIOCGIFSTATS, &ifr) < 0)
		memset (stats, 0, sizeof (*stats));
	else
		*stats = ifr.ifr_stats;
}

static long
get_mtu_metric (char *name, short which)
{
	struct ifreq ifr;

	strcpy (ifr.ifr_name, name);
	if (ioctl (sock, which, &ifr) < 0)
	{
		fprintf (stderr, "%s: cannot get %s: %s\n",
			name,
			which == SIOCGIFMETRIC ? "METRIC" : "MTU",
			strerror (errno));
		exit (1);
	}

	return ifr.ifr_metric;
}

static void
set_mtu_metric (char *name, short which, long val)
{
	struct ifreq ifr;

	strcpy (ifr.ifr_name, name);
	ifr.ifr_metric = val;
	if (ioctl (sock, which, &ifr) < 0)
	{
		fprintf (stderr, "%s: cannot set %s: %s\n",
			name,
			which == SIOCSIFMETRIC ? "METRIC" : "MTU",
			strerror (errno));
		exit (1);
	}
}

static short
get_flags (char *name)
{
	struct ifreq ifr;

	strcpy (ifr.ifr_name, name);
	if (ioctl (sock, SIOCGIFFLAGS, &ifr) < 0)
	{
		fprintf (stderr, "%s: cannot get FLAGS: %s\n",
			name,
			strerror (errno));
		exit (1);
	}

	return (ifr.ifr_flags);
}

static void
set_flags (char *name, short flags)
{
	struct ifreq ifr;

	strcpy (ifr.ifr_name, name);
	ifr.ifr_flags = flags;
	if (ioctl (sock, SIOCSIFFLAGS, &ifr) < 0)
	{
		fprintf (stderr, "%s: cannot set FLAGS: %s\n",
			name,
			strerror (errno));
		if (errno == ENODEV && (flags & (IFF_UP|IFF_RUNNING)) == IFF_UP)
			fprintf (stderr, "hint: probably the interface is "
				"not linked to a device. Use iflink!\n");
		exit (1);
	}
}

/*
 * Get interface link level flags
 */
static int
get_lnkflags (char *ifname, short *flags)
{
	struct ifreq ifr;

	strcpy (ifr.ifr_name, ifname);
	if (ioctl (sock, SIOCGLNKFLAGS, &ifr) < 0)
		return -1;

	*flags = ifr.ifr_flags;
	return 0;
}

/*
 * Set interface link level flags
 */
static int
set_lnkflags (char *ifname, short flags)
{
	struct ifreq ifr;

	strcpy (ifr.ifr_name, ifname);
	ifr.ifr_flags = flags;

	if (ioctl (sock, SIOCSLNKFLAGS, &ifr) < 0)
		return -1;

	return 0;
}

static void
set_addr (char *name, short which, unsigned long addr)
{
	struct ifreq ifr;
	struct sockaddr_in in;

	in.sin_family = AF_INET;
	in.sin_addr.s_addr = addr;
	in.sin_port = 0;
	strcpy (ifr.ifr_name, name);
	memcpy (&ifr.ifr_addr, &in, sizeof (in));

	if (ioctl (sock, which, &ifr) < 0)
	{
		fprintf (stderr, "%s: cannot set %s: %s\n",
			name,
			which == SIOCSIFMETRIC ? "ADDRESS" :
			which == SIOCSIFBRDADDR ? "BROADCAST ADDRESS" :
			which == SIOCSIFDSTADDR ? "DESTINATION ADDRESS" :
			"NETMASK",
			strerror (errno));
		exit (1);
	}
}

static unsigned long
get_addr (char *name, short which)
{
	struct ifreq ifr;

	strcpy (ifr.ifr_name, name);
	ifr.ifr_addr.sa_family = AF_INET;
	if (ioctl (sock, which, &ifr) < 0)
	{
		fprintf (stderr, "%s: cannot get %s: %s\n",
			name,
			which == SIOCGIFMETRIC ? "ADDRESS" :
			which == SIOCGIFBRDADDR ? "BROADCAST ADDRESS" :
			which == SIOCGIFDSTADDR ? "DESTINATION ADDRESS" :
			"NETMASK",
			strerror (errno));
		exit (1);
	}

	return ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr;
}

static char *
decode_flags (short flags)
{
	static char str[256];

	str[0] = '\0';

	if (flags & IFF_UP)
		strcat (str, "UP,");
	if (flags & IFF_BROADCAST)
		strcat (str, "BROADCAST,");
	if (flags & IFF_DEBUG)
		strcat (str, "DEBUG,");
	if (flags & IFF_LOOPBACK)
		strcat (str, "LOOPBACK,");
	if (flags & IFF_POINTOPOINT)
		strcat (str, "POINTOPOINT,");
	if (flags & IFF_NOTRAILERS)
		strcat (str, "NOTRAILERS,");
	if (flags & IFF_RUNNING)
		strcat (str, "RUNNING,");
	if (flags & IFF_NOARP)
		strcat (str, "NOARP,");

	if (strlen (str))
		str[strlen (str) - 1] = '\0';

	return str;
}

static void
print_if (char *name)
{
	short lflags, flags;
	char *sflags;
	struct in_addr addr;
	long mtu_metric;
	struct ifstat stats;

	flags = get_flags (name);
	sflags = decode_flags (flags);
	printf ("%s:\tflags=0x%x<%s>", name, flags, sflags);

	if (get_lnkflags (name, &lflags) == 0)
		printf ("\n\tlink-level-flags=0x%x", lflags);

	if (flags & IFF_UP)
	{
		printf ("\n\t");
		addr.s_addr = get_addr (name, SIOCGIFADDR);
		printf ("inet %s ", inet_ntoa (addr));

		addr.s_addr = get_addr (name, SIOCGIFNETMASK);
		printf ("netmask %s ", inet_ntoa (addr));

		if (flags & IFF_POINTOPOINT)
		{
			addr.s_addr = get_addr (name, SIOCGIFDSTADDR);
			printf ("dstaddr %s ", inet_ntoa (addr));
		}

		if (flags & IFF_BROADCAST)
		{
			addr.s_addr = get_addr (name, SIOCGIFBRDADDR);
			printf ("broadcast %s ", inet_ntoa (addr));
		}
	}

	printf ("\n\t");
	mtu_metric = get_mtu_metric (name, SIOCGIFMETRIC);
	printf ("metric %ld ", mtu_metric);

	mtu_metric = get_mtu_metric (name, SIOCGIFMTU);
	printf ("mtu %ld\n\t", mtu_metric);

	get_stats (name, &stats);
	printf ("in-packets  %lu in-errors  %lu collisions %lu\n\t",
		stats.in_packets, stats.in_errors, stats.collisions);
	printf ("out-packets %lu out-errors %lu\n",
		stats.out_packets, stats.out_errors);
}

static void
list_all_if (short all)
{
	struct ifconf ifc;
	struct ifreq ifr[50];
	int i, n;

	ifc.ifc_len = sizeof (ifr);
	ifc.ifc_req = ifr;
	if (ioctl (sock, SIOCGIFCONF, &ifc) < 0)
	{
		perror ("cannot get interface list");
		exit (1);
	}

	n = ifc.ifc_len / sizeof (struct ifreq);
	for (i = 0; i < n; ++i)
	{
		if (ifr[i].ifr_addr.sa_family != AF_INET)
			continue;

		if (all || (get_flags (ifr[i].ifr_name) & IFF_UP))
			print_if (ifr[i].ifr_name);
	}
}

static void
usage (void)
{
	printf ("ifconfig [-a|-v] [<interface name>]\n");
	printf ("\t [addr <local address>] [netmask aa.bb.cc.dd]\n");
	printf ("\t [dstaddr <point to point destination address>]\n");
	printf ("\t [broadaddr aa.bb.cc.dd]\n");
	printf ("\t [up|down|[-]arp|[-]trailers|[-]debug]\n");
	printf ("\t [mtu NN] [metric NN]\n");
	printf ("\t [linkNN] [-linkNN]\n");
	printf ("\t [-f <option file>]\n");
	exit (0);
}

int
main (int argc, char *argv[])
{
#define NEXTARG(i)	{ if (++i >= argc) usage (); }
	int i, show_all = 0;
	char *ifname = NULL;
	unsigned long addr;
	short flags, lflags;

	sock = socket (PF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
	{
		perror ("cannot open socket");
		exit (1);
	}

	for (i = 1; i < argc; ++i)
	{
		if (!strcmp (argv[i], "-a"))
			show_all = 1;
		else if (!strcmp (argv[i], "-v"))
			usage ();
		else
		{
			ifname = argv[i++];
			break;
		}
	}

	if (!ifname)
		list_all_if (show_all);
	else if (i == argc)
		print_if (ifname);
	else
	{
		flags = get_flags (ifname);
		lflags = 0;
		get_lnkflags (ifname, &lflags);

		for (; i < argc; ++i)
		{
			if (!strcmp (argv[i], "addr"))
			{
				struct hostent *hent;
				NEXTARG (i);
				hent = gethostbyname (argv[i]);
				if (!hent)
				{
					fprintf (stderr, "cannot lookup host %s",
						argv[i]);
					herror ("");
					exit (1);
				}
				memcpy (&addr, hent->h_addr, hent->h_length);
				set_addr (ifname, SIOCSIFADDR, addr);
				flags |= IFF_UP;
			}
			else if (!strcmp (argv[i], "broadaddr"))
			{
				NEXTARG (i);
				addr = inet_addr (argv[i]);
				set_addr (ifname, SIOCSIFBRDADDR, addr);
			}
			else if (!strcmp (argv[i], "dstaddr"))
			{
				struct hostent *hent;
				NEXTARG (i);
				hent = gethostbyname (argv[i]);
				if (!hent)
				{
					fprintf (stderr, "cannot lookup host %s",
						argv[i]);
					herror ("");
					exit (1);
				}
				memcpy (&addr, hent->h_addr, hent->h_length);
				set_addr (ifname, SIOCSIFDSTADDR, addr);
			}
			else if (!strcmp (argv[i], "netmask"))
			{
				NEXTARG (i);
				addr = inet_addr (argv[i]);
				set_addr (ifname, SIOCSIFNETMASK, addr);
			}
			else if (!strcmp (argv[i], "up"))
			{
				flags |= IFF_UP;
			}
			else if (!strcmp (argv[i], "down"))
			{
				flags &= ~IFF_UP;
			}
			else if (!strcmp (argv[i], "arp"))
			{
				flags &= ~IFF_NOARP;
			}
			else if (!strcmp (argv[i], "-arp"))
			{
				flags |= IFF_NOARP;
			}
			else if (!strcmp (argv[i], "trailers"))
			{
				flags &= ~IFF_NOTRAILERS;
			}
			else if (!strcmp (argv[i], "-trailers"))
			{
				flags |= IFF_NOTRAILERS;
			}
			else if (!strcmp (argv[i], "debug"))
			{
				flags |= IFF_DEBUG;
			}
			else if (!strcmp (argv[i], "-debug"))
			{
				flags &= ~IFF_DEBUG;
			}
			else if (!strcmp (argv[i], "mtu"))
			{
				NEXTARG (i);
				set_mtu_metric (ifname, SIOCSIFMTU,
					atol (argv[i]));
			}
			else if (!strcmp (argv[i], "metric"))
			{
				NEXTARG (i);
				set_mtu_metric (ifname, SIOCSIFMETRIC,
					atol (argv[i]));
			}
			else if (!strncmp (argv[i], "link", 4))
			{
				int bit = atoi (&argv[i][4]);
				if (bit < 0 || bit > 15)
					usage ();
				lflags |= (1 << bit);
			}
			else if (!strncmp (argv[i], "-link", 5))
			{
				int bit = atoi (&argv[i][5]);
				if (bit < 0 || bit > 15)
					usage ();
				lflags &= ~(1 << bit);
			}
			else if (!strcmp (argv[i], "-f"))
			{
				NEXTARG (i);
				opt_file (argv[i], ifname, sock);
			}
			else
			{
				fprintf (stderr, "unknown option %s\n",
					argv[i]);
				usage ();
			}
		}

		set_flags (ifname, flags);
		set_lnkflags (ifname, lflags);
	}

	return 0;
}
