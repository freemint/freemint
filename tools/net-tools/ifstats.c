/*
 * ifstats(8) (interface statistics) for MintNet. (w) 1994 Kay Roemer.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sockios.h>


static void
slcomp_stats (unsigned long *l, char *name)
{
	printf ("VJ compression statistics for interface %s:\n", name);
	printf ("%10lu total packets output\n", *l++);
	printf ("%10lu compressed packets output\n", *l++);
	printf ("%10lu searches for connection state\n", *l++);
	printf ("%10lu misses while searching for conn. state\n", *l++);
	printf ("%10lu uncompressed packets input\n", *l++);
	printf ("%10lu compressed packets input\n", *l++);
	printf ("%10lu packets with unknown type input\n", *l++);
	printf ("%10lu tossed input packets\n", *l++);
}

static void
ether_stats (unsigned long *l, char *name)
{
	printf ("Statistics for Ethernet interface %s:\n", name);
}

static void
ppp_stats (unsigned long *l, char *name)
{
	printf ("Statistics for PPP interface %s:\n", name);
	printf ("%10lu IP packets input\n", *l++);
	printf ("%10lu other packets input\n", *l++);
	printf ("%10lu to long input packets dropped\n", *l++);
	printf ("%10lu to short input packets dropped\n", *l++);
	printf ("%10lu input packets with bad checksum dropped\n", *l++);
	printf ("%10lu input packets with bad enconding dropped\n", *l++);
	printf ("%10lu bad VJ compressed input packets dropped\n", *l++);
	printf ("%10lu input packets dropped while queue full\n", *l++);
	printf ("%10lu input packets while out of memory\n", *l++);
	printf ("%10lu IP packets output\n", *l++);
	printf ("%10lu other packets output\n", *l++);
	printf ("%10lu output packets dropped while queue full\n", *l++);
	printf ("%10lu output packets dropped while out of memory\n", *l++);
	slcomp_stats (l, name);
}

static void
slip_stats (long *l, char *name)
{
	printf ("Statistics for SLIP interface %s:\n", name);
	printf ("%10lu input packets\n", *l++);
	printf ("%10lu to short input packets dropped\n", *l++);
	printf ("%10lu to long input packets dropped\n", *l++);
	printf ("%10lu input packets with bad encoding dropped\n", *l++);
	printf ("%10lu bad VJ compressed input packets dropped\n", *l++);
	printf ("%10lu input packets dropped while out of memory\n", *l++);
	printf ("%10lu input packets dropped while queue full\n", *l++);
	printf ("%10lu output packets\n", *l++);
	printf ("%10lu output packets dropped while queue full\n", *l++);
	slcomp_stats (l, name);
}

static void
usage (void)
{
	printf ("usage:\t ifstats <interface name>\n");
	exit (1);
}

int
main (int argc, char *argv[])
{
	static struct ifreq ifr;
	static unsigned long stats[40];
	int sockfd;

	if (argc != 2)
		usage ();

	sockfd = socket (PF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
	{
		perror ("cannot open socket");
		return 1;
	}

	strcpy (ifr.ifr_name, argv[1]);
	ifr.ifr_data = stats;
	if (ioctl (sockfd, SIOCGLNKSTATS, &ifr) < 0)
	{
		if (errno == EINVAL)
			fprintf (stderr, "no statistics for %s available\n",
				argv[1]);
		else
			perror ("cannot get interface statistics");
		return 1;
	}

	if (!strncmp (argv[1], "en", 2))
		ether_stats (stats, argv[1]);
	else if (!strncmp (argv[1], "sl", 2))
		slip_stats (stats, argv[1]);
	else if (!strncmp (argv[1], "ppp", 3))
		ppp_stats (stats, argv[1]);
	else
		fprintf (stderr, "no statistics for %s available\n", argv[1]);

	return 0;
}
