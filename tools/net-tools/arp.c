/*
 * arp(8) ARP/RARP cache management utility for MintNet. (w) '94 Kay Roemer.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/if_ether.h>
#include <sockios.h>


#define _PATH_DEV_ARP	"/dev/arp"

static int sockfd;

/*
 * structure that can be read from /dev/arp
 */
struct arp_info
{
	struct sockaddr_hw	praddr;
	struct sockaddr_hw	hwaddr;
	unsigned short		flags;
	unsigned short		links;
	unsigned long		tmout;
};

static int arpfd = -1;
static struct arp_info arpinfo;

static void
setarpent (void)
{
	if (arpfd < 0)
		arpfd = open (_PATH_DEV_ARP, O_RDONLY);
}

static void
endarpent (void)
{
	if (arpfd >= 0)
		close (arpfd);

	arpfd = -1;
}

static struct arp_info *
getarpent (void)
{
	int r;

	if (arpfd < 0)
		return 0;

	r = read (arpfd, &arpinfo, sizeof (arpinfo));
	if (r != sizeof (arpinfo))
		return 0;

	return &arpinfo;
}

static char *
decode_hw (struct sockaddr_hw *shw)
{
	static char strbuf[20];
	unsigned char *cp;

	strcpy (strbuf, "unknown");
	if (shw->shw_family == AF_LINK)
	{
		switch (shw->shw_type)
		{
		case ARPHRD_ETHER:
			if (shw->shw_len != 6)
				break;
			cp = shw->shw_addr;
			sprintf (strbuf, "%x:%x:%x:%x:%x:%x", cp[0], cp[1], cp[2],
				cp[3], cp[4], cp[5]);
			break;
		}
	}

	return strbuf;
}

static char *
decode_pr (shw)
	struct sockaddr_hw *shw;
{
	static char strbuf[20];
	struct in_addr ina;

	strcpy (strbuf, "unknown");
	if (shw->shw_family == AF_LINK) switch (shw->shw_type) {
	case ETHERTYPE_IP:
		if (shw->shw_len != 4)
			break;
		ina.s_addr = *(long *)shw->shw_addr;
		strcpy (strbuf, inet_ntoa (ina));
		break;
	}

	return strbuf;
}

static char *
decode_tmout (unsigned long tmout)
{
	static char strbuf[20];
	sprintf (strbuf, "%lds", tmout/1000);
	return strbuf;
}

static char *
decode_flags (unsigned short flags)
{
	static char strbuf[100];

	strbuf[0] = '\0';

	if (flags & ATF_PRCOM)
		strcat (strbuf, "PRCOM,");
	if (flags & ATF_COM)
		strcat (strbuf, "HWCOM,");
	if (flags & ATF_PERM)
		strcat (strbuf, "PERM,");
	if (flags & ATF_PUBL)
		strcat (strbuf, "PUBL,");
	if (flags & ATF_USETRAILERS)
		strcat (strbuf, "TRAIL,");
	if (flags & ATF_NORARP)
		strcat (strbuf, "NORARP,");

	if (strlen (strbuf))
		strbuf[strlen (strbuf) - 1] = '\0';

	return strbuf;
}

#define HEAD "Proto Address    Hardware Address    Ref    Tmout  Flags"
#define LINE "%-16s %-17s %5d %8s  %s\n"

static void
show (void)
{
	struct arp_info *ai;

	setarpent ();

	puts (HEAD);
	while ((ai = getarpent ()))
	{
		printf (LINE,
			ai->flags & ATF_PRCOM ? decode_pr (&ai->praddr) : "",
			ai->flags & ATF_COM   ? decode_hw (&ai->hwaddr) : "",
			ai->links,
			decode_tmout (ai->tmout),
			decode_flags (ai->flags));
	}

	endarpent ();
}

static int
usage (void)
{
	printf ("usage:\t arp\n");
	printf ("\t arp del <ip address>\n");
	printf ("\t arp add <ip address> <hw address> [perm][publ][norarp]\n");
	exit (1);
}

static int
get_hwaddr (struct sockaddr_hw *shw, char *addr)
{
	static char str[20];
	unsigned int x, i;
	char *cp;

	shw->shw_family = AF_LINK;
	shw->shw_type = ARPHRD_ETHER;
	shw->shw_len = 6;
	strncpy (str, addr, sizeof (str));
	cp = strtok (str, ":");
	for (i = 0; i < 6 && cp; ++i, cp = strtok (NULL, ":"))
	{
		if (sscanf (cp, "%x", &x) != 1 || x > 255)
			goto bad;
		shw->shw_addr[i] = (char)x;
	}

	if (i < 6 || cp)
		goto bad;

	return 0;

bad:
	fprintf (stderr, "invalid hardware address: %s\n", addr);
	return -1;
}

static int
add (int argc, char *argv[])
{
	struct arpreq areq;
	struct sockaddr_hw *shw = (struct sockaddr_hw *) &areq.arp_ha;
	struct sockaddr_in *in = (struct sockaddr_in *) &areq.arp_pa;

	if (argc < 2)
		usage ();

	memset (&areq, 0, sizeof (areq));

	in->sin_family = AF_INET;
	in->sin_addr.s_addr = inet_addr (argv[0]);
	if (in->sin_addr.s_addr == INADDR_NONE)
	{
		fprintf (stderr, "invalid ip address: %s\n", argv[0]);
		exit (1);
	}

	if (get_hwaddr (shw, argv[1]))
		exit (1);

	for (argv += 2; *argv; ++argv)
	{
		if (!strcmp ("perm", *argv))
			areq.arp_flags |= ATF_PERM;
		else if (!strcmp ("publ", *argv))
			areq.arp_flags |= ATF_PUBL;
		else if (!strcmp ("norarp", *argv))
			areq.arp_flags |= ATF_NORARP;
		else
		{
			fprintf (stderr, "invalid option: %s\n", *argv);
			exit (1);
		}
	}

	if (ioctl (sockfd, SIOCSARP, &areq) < 0)
	{
		perror ("cannot add arp entry");
		exit (1);
	}

	return 0;
}

static int
del (int argc, char *argv[])
{
	struct arpreq areq;
	struct sockaddr_in *in = (struct sockaddr_in *) &areq.arp_pa;

	if (argc != 1)
		usage ();

	memset (&areq, 0, sizeof (areq));

	in->sin_family = AF_INET;
	in->sin_addr.s_addr = inet_addr (argv[0]);
	if (in->sin_addr.s_addr == INADDR_NONE)
	{
		fprintf (stderr, "invalid ip address: %s\n", argv[0]);
		exit (1);
	}

	if (ioctl (sockfd, SIOCDARP, &areq) < 0)
	{
		perror ("cannot add arp entry");
		exit (1);
	}

	return 0;
}

int
main (int argc, char *argv[])
{
	sockfd = socket (PF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
	{
		perror ("cannot create socket");
		return 1;
	}

	if (argc == 1)
		show ();
	else if (!strcmp ("add", argv[1]))
		add (argc-2, &argv[2]);
	else if (!strcmp ("del", argv[1]))
		del (argc-2, &argv[2]);
	else
		usage ();

	return 0;
}
