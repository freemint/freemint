/*
 *	slattach(8) utility for MintNet (w) 1994, Kay Roemer.
 *	baud rate setting implemented by Borek Lumpovsky.
 *
 *	Options:
 *
 *	-h			Don't hang the line up when exiting.
 *	-e			Exit after setting up the connection.
 *	-d			Don't add default route.
 *	-t <terminal line>	Specify terminal device to use. If this
 *				option is missing use stdin instead.
 *      -s <speed>              Specify baudrate
 *	-r <remote host>	Specify the IP address of the remote host.
 *	-l <local host>		Specify the IP address of the local host.
 *	-p <protcol>		Specify protocol type (slip, cslip or ppp).
 *	-c			Turn on RTSCTS.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_sl.h>
#include <netinet/in.h>
#include <support.h>
#include <netdb.h>
#include <osbind.h>
#include <sockios.h>


/* FIXME:  Is this necessary?  */
#define _PATH_IFCONFIG	"/sbin/ifconfig"
#define _PATH_ROUTE	"/sbin/route"

#define FLOW_HARD	2


struct bauds
{
	long	rate;
	char	tty_val;
	int	rsconf_val;
};

static struct bauds baudstab [] =
{
	{ 50,		B50,	15 },
	{ 75,		B75,	14 },
	{ 110,		B110,	13 },
	{ 134,		B134,	12 },
	{ 150,		B150,	11 },
	{ 200,		B200,	10 },
	{ 300,		B300,	9  },
	{ 600,		B600,	8  },
	{ 1200,		B1200,	7  },
	{ 1800,		B1800,	6  },
	{ 2400,		B2400,	4  },
	{ 4800,		B4800,	2  },
	{ 9600,		B9600,	1  },
	{ 19200,	B19200,	0  },
	{ 38400,	B110,	13 },
	{ 57600,	B134,	12 },
	{ 115200,	B150,	11 },
	{ 153600,	B75,	14 }
};

#define BAUDS	18

static char ifname[16];
static int exit_opt = 0, hangup_opt = 1, defrt_opt = 1;
static int rtscts_opt = 0;
static struct sgttyb savesg;


/*
 * simple system() without shell invocation.
 */

static int
mysystem (char *command, ...)
{
	int status;
	pid_t pid;

	pid = vfork ();
	if (pid < 0)
	{
		perror ("fork");
		exit (1);
	}

	if (pid == 0)
	{
		execv (command, &command);
		fprintf (stderr, "cannot execute %s: %s\n", command, strerror (errno));
		exit (1);
	}

	if (waitpid (pid, &status, 0) < 0)
	{
		perror ("waitpid");
		exit (1);
	}

	return WIFEXITED (status) ? WEXITSTATUS (status) : 0;
}

/*
 * link device to network interface.
 */
static void
do_link (char *device, char *name)
{
	extern int __libc_unix_names;  /* Secret MiNTLib feature.  */
	struct iflink ifl;
	int sockfd;
	long r;

	sockfd = socket (PF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
	{
		perror ("cannot open socket");
		exit (1);
	}

	if (!__libc_unix_names)
		unx2dos (device, ifl.device);
	else
		strcpy (device, ifl.device);

	strncpy (ifl.ifname, name, sizeof (ifl.ifname));
	r = ioctl (sockfd, SIOCSIFLINK, &ifl);
	if (r < 0)
	{
		fprintf (stderr, "cannot link %s to an interface: %s\n", device, strerror (errno));
		close (sockfd);
		exit (1);
	}

	close (sockfd);
	strncpy (ifname, ifl.ifname, sizeof (ifl.ifname));
}

/*
 * Setup routines for PPP, SLIP, (CSLIP).
 */

static int
ppp_init (char *tty, char *iname)
{
	strcpy (iname, "ppp");
	do_link (tty, iname);
	return 0;
}

static int
slip_init (char *tty, char *iname)
{
	struct ifreq ifr;
	int sock;

	strcpy (iname, "sl");
	do_link (tty, iname);

	sock = socket (PF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
	{
		perror ("cannot open socket");
		goto error;
	}

	strcpy (ifr.ifr_name, iname);
	if (ioctl (sock, SIOCGLNKFLAGS, &ifr) < 0)
	{
		fprintf (stderr, "%s: cannot get LINK LEVEL FLAGS: %s\n", iname, strerror (errno));
		goto error;
	}

	ifr.ifr_flags &= ~SLF_COMPRESS;
	ifr.ifr_flags |= SLF_AUTOCOMP;
	if (ioctl (sock, SIOCSLNKFLAGS, &ifr) < 0)
	{
		fprintf (stderr, "%s: cannot set LINK LEVEL FLAGS: %s\n", iname, strerror (errno));
		goto error;
	}

	close (sock);
	return 0;

error:
	if (sock >= 0) close (sock);
	return -1;
}

static int
cslip_init (char *tty, char *iname)
{
	struct ifreq ifr;
	int sock;

	strcpy (iname, "sl");
	do_link (tty, iname);

	sock = socket (PF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
	{
		perror ("cannot open socket");
		goto error;
	}

	strcpy (ifr.ifr_name, iname);
	if (ioctl (sock, SIOCGLNKFLAGS, &ifr) < 0)
	{
		fprintf (stderr, "%s: cannot get LINK LEVEL FLAGS: %s\n", iname, strerror (errno));
		goto error;
	}

	ifr.ifr_flags |= SLF_COMPRESS;
	if (ioctl (sock, SIOCSLNKFLAGS, &ifr) < 0)
	{
		fprintf (stderr, "%s: cannot set LINK LEVEL FLAGS: %s\n", iname, strerror (errno));
		goto error;
	}

	close (sock);
	return 0;

error:
	close (sock);
	return -1;
}

struct protocol
{
	char	*name;
	int	(*init) (char *tty, char *iname);
};

static struct protocol allprotos [] =
{
	{ "slip",  slip_init  },
	{ "ppp",   ppp_init   },
	{ "cslip", cslip_init },
	{ 0, 0 }
};

static int
if_setup (char *tty, char *proto, char *iname)
{
	struct protocol *p;

	for (p = allprotos; p->name; ++p)
		if (!stricmp (proto, p->name))
			return (*p->init) (tty, iname);

	fprintf (stderr, "if_setup: %s: unknown protocol\n", proto);
	return -1;
}

/*
 * TTY stuff.
 */

static int
set_baud (int ttyfd, int fake, int baudidx)
{
	struct sgttyb sg;

	/*
	 * Changing baud using terminal file descriptor and stty()
	 */
	if (gtty (ttyfd, &sg) < 0)
	{
		perror ("cannot get terminal parameters");
		return -1;
	}

	sg.sg_ospeed = sg.sg_ispeed = baudstab[baudidx].tty_val;
	if (stty (ttyfd, &sg) < 0)
	{
		perror ("cannot set terminal parameters");
		return -1;
	}

	/*
	 * Is this really needed?
	 */
#if 0
	/*
	 * Changing baud using Rsconf() and Bconmap(); required with
	 * HSmodem with speeds above 19200 !!!
	 */
	{
		struct stat st;
		int biosdev, save = 0;

		if (baudstab[baudidx].rate > 19200)
		{
			if (fstat (ttyfd, &st) < 0 || !S_ISCHR (st.st_mode))
			{
				fprintf (stderr, "warning: can't set speed, no tty!\n");
				return 0;
			}

			biosdev = st.st_rdev & 0xff;
			if (biosdev != 1)
				save = Bconmap (biosdev);

			Rsconf (baudstab[baudidx].rsconf_val, FLOW_HARD, -1, -1, -1, -1);
			if (biosdev != 1)
				Bconmap (save);
		}
	}
#endif

	return 0;
}

static void
set_raw_tty (int fd)
{
	int r;
	struct sgttyb sg;

	r = gtty (fd, &sg);
	if (r < 0)
	{
		perror ("cannot get terminal flags");
		exit (1);
	}

	savesg = sg;
	sg.sg_flags |= RAW;
	if (rtscts_opt) sg.sg_flags |= RTSCTS;
	sg.sg_flags &= ~(CBREAK|ECHO|CRMOD|XKEY|TANDEM|EVENP|ODDP|TOSTOP);

	r = stty (fd, &sg);
	if (r < 0)
	{
		perror ("cannot set terminal flags");
		exit (1);
	}
}

static void
set_canon_tty (int fd)
{
	int r;

	r = stty (fd, &savesg);
	if (r < 0)
	{
		perror ("cannot set teminal flags");
		exit (1);
	}
}

static int ttyfd;

static void
hangup (void)
{
	struct timeval tm = { 0, 600000L };
	struct sgttyb sg;
	char ispeed, ospeed;

	if (ioctl (ttyfd, TIOCGETP, &sg) < 0)
	{
		perror ("cannot hangup the line");
		return;
	}

	ispeed = sg.sg_ispeed;
	ospeed = sg.sg_ospeed;
	sg.sg_ispeed = B0;
	sg.sg_ospeed = B0;

	if (ioctl (ttyfd, TIOCSETN, &sg) < 0)
	{
		perror ("cannot hangup the line");
		return;
	}

	select (0, 0, 0, 0, &tm);
	sg.sg_ispeed = ispeed;
	sg.sg_ospeed = ospeed;

	if (ioctl (ttyfd, TIOCSETN, &sg) < 0)
	{
		perror ("cannot hangup the line");
		return;
	}
}

static void
assert_dtr (void)
{
	struct sgttyb sg;

	if (ioctl (ttyfd, TIOCGETP, &sg) < 0)
	{
		perror ("cannot raise DTR");
		return;
	}

	if (sg.sg_ispeed == B0)
		sg.sg_ispeed = B9600;

	if (sg.sg_ospeed == B0)
		sg.sg_ospeed = B9600;

	if (ioctl (ttyfd, TIOCSETN, &sg) < 0)
	{
		perror ("cannot raise DTR");
		return;
	}
}

static void
sig_handler (int sig)
{
	long r;

	set_canon_tty (ttyfd);

	r = mysystem (_PATH_IFCONFIG, ifname, "down", NULL);
	if (r != 0)
		fprintf (stderr, "cannot shut down interface %s\n", ifname);

	if (hangup_opt)
		hangup ();

	signal (SIGINT, SIG_DFL);
	raise (SIGINT);
}

static char *message =
"usage: slattach [options] -r <remote host> -l <local host>\n"
"options:\n"
"\t[-t <tty device>] [-p {ppp, slip, cslip}]\n"
"\t[-s <baudrate>]\n"
"\t[-h] [-e] [-d]\n";

static void
usage (void)
{
	printf ("%s", message);
	exit (1);
}

int
main (int argc, char *argv[])
{
	char *tty = NULL, *lhost = NULL, *rhost = NULL, *proto = "slip";
	long baud = -1, pgrp;
	int baudidx;
	extern char *optarg;
	int c, r;

	while ((c = getopt (argc, argv, "cdhes:l:r:p:t:")) != EOF)
	{
		switch (c)
		{
			case 'c':
				rtscts_opt = 1;
				break;

			case 't':
				tty = optarg;
				break;

			case 's':
				baud = atol(optarg);
				break;

			case 'r':
				rhost = optarg;
				break;

			case 'l':
				lhost = optarg;
				break;

			case 'e':
				exit_opt = 1;
				break;

			case 'h':
				hangup_opt = 0;
				break;

			case 'd':
				defrt_opt = 0;
				break;		

			case 'p':
				proto = optarg;
				break;

			case '?':
				usage ();
				break;
		}
	}

	if (tty == NULL)
	{
		tty = ttyname (0);
		ttyfd = 0;
	}
	else
	{
		ttyfd = open (tty, O_RDONLY);
		if (ttyfd < 0)
		{
			fprintf (stderr, "cannot open %s: %s\n", tty, strerror (errno));
			exit (1);
		}
	}

	if (lhost == NULL || rhost == NULL || tty == NULL)
		usage ();

	signal (SIGQUIT, sig_handler);
	signal (SIGINT, sig_handler);
	signal (SIGTERM, sig_handler);
	signal (SIGHUP, sig_handler);

	/*
	 * Raise DTR and make tty raw
	 */
	assert_dtr ();
	set_raw_tty (ttyfd);

	pgrp = getpgrp();
	ioctl (ttyfd, TIOCSPGRP, &pgrp);
	ioctl (ttyfd, TIOCCAR, 0);

	/*
	 * Check for validity of -s switch and set baud rate.
	 */
	if (baud != -1)
	{
		for (baudidx=0; baudidx<BAUDS; baudidx++)
			if (baudstab[baudidx].rate == baud)
				break;

		if (baudidx == BAUDS)
		{
			fprintf (stderr, "slattach: invalid baudrate %ld\n", baud);
			goto bad;
		}

		if (set_baud (ttyfd, 0, baudidx) < 0)
		{
			fprintf (stderr, "slattach: cannot set baudrate\n");
			goto bad;
		}
	}

	/*
	 * Link device 'tty' to a free network interface of type 'proto'.
	 * Leaves actual interface name in 'ifname'.
	 */
	if (if_setup (tty, proto, ifname) < 0)
		goto bad;

	/*
	 * Setup the addresses of the interface using ifconfig.
	 */
	r = mysystem (_PATH_IFCONFIG, ifname, "addr", lhost, "dstaddr", rhost, "up", NULL);
	if (r != 0)
	{
		fprintf (stderr, "cannot activate interface %s\n", ifname);
		goto bad;
	}

	/*
	 * Install route to remote system.
	 */
	r = mysystem (_PATH_ROUTE, "add", rhost, ifname, NULL);
	if (r != 0)
	{
		fprintf (stderr, "cannot install routing entry\n");
		goto bad;
	}

	/*
	 * If appropriate install default route to go over remote.
	 */
	if (defrt_opt)
	{
		r = mysystem (_PATH_ROUTE, "add", "default", ifname, "gw", rhost, NULL);
		if (r != 0)
		{
			fprintf (stderr, "cannot install default route\n");
			goto bad;
		}
	}

	/*
	 * Wait for shutdown
	 */
	if (!exit_opt)
	{
		for (;;)
			pause ();
	}

	return 0;

bad:
	set_canon_tty (ttyfd);
	return 1;
}
