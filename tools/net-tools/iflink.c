/*
 *	iflink(8) utilitiy for MintNet (w) 1994, Kay Roemer.
 *
 *	Options:
 *
 *	-i <interface name>	Specify the interface name (unit number
 *				is isgnored if specified at all) to which
 *				the device should be linked.
 *
 *	-d <device path>	Specify the path of the device which should
 *				be linked to the network interface.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sockios.h>

#include <support.h>


static int sockfd;

static void
do_link (char *device, char *ifname)
{
	struct iflink ifl;
	long r;

	_unx2dos (device, ifl.device, sizeof (ifl.device));
	strncpy (ifl.ifname, ifname, sizeof (ifl.ifname));
	r = ioctl (sockfd, SIOCSIFLINK, &ifl);
	if (r < 0)
	{
		fprintf (stderr, "cannot link %s to an interface: %s\n",
				device, strerror (errno));
		exit (1);
	}

	printf ("%s\n", ifl.ifname);
}

static void
get_device (char *ifname)
{
	struct iflink ifl;
	char device[sizeof (ifl.device)];
	long r;

	strncpy (ifl.ifname, ifname, sizeof (ifl.ifname));
	r = ioctl (sockfd, SIOCGIFNAME, &ifl);
	if (r < 0)
	{
		if (errno == EINVAL)
			fprintf (stderr, "%s: not linked to any device\n",
				ifname);
		else
			fprintf (stderr, "%s: cannot get the device linked to "
				"this interface: %s\n",
				ifname, strerror (errno));
		exit (1);
	}

	_dos2unx (ifl.device, device, sizeof (device));
	printf ("%s\n", device);
}

static void
usage (void)
{
	printf ("usage: iflink -i <interface> [-d <device>]\n");
	exit (1);
}

int
main (int argc, char *argv[])
{
	char *device = NULL, *ifname = NULL;
	int c;

	sockfd = socket (PF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
	{
		perror ("cannot open socket");
		exit (1);
	}

	while ((c = getopt (argc, argv, "i:d:")) != EOF)
	{
		switch (c)
		{
			case 'i':
				ifname = optarg;
				break;

			case 'd':
				device = optarg;
				break;

			case '?':
				usage ();
				break;
		}
	}

	if (device && ifname)
		do_link (device, ifname);
	else if (ifname)
		get_device (ifname);
	else
		usage ();

	return 0;
}
