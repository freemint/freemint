
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


static int fd;

static void
oob (int sig)
{
	static char buf[10];
	int r, atmark;
	fd_set xset;
	struct timeval tm = { 0, 0 };

	do {
		printf ("DATA: ");
		fflush (stdout);
		while (1)
		{
			r = ioctl (fd, SIOCATMARK, &atmark);
			if (r < 0)
			{
				perror ("ioctl SIOCATMARK");
				break;
			}
			if (atmark) break;
			r = read (fd, buf, sizeof (buf) - 1);
			if (r < 0)
			{
				perror ("read");
				break;
			}
			else if (r == 0)
			{
				raise (SIGQUIT);
				return;
			}
			buf[r] = '\0';
			printf ("%s", buf);
			fflush (stdout);
		}
		puts ("");

		r = recv (fd, buf, sizeof (buf) - 1, MSG_OOB);
		if (r < 0)
		{
			perror ("recv");
			return;
		}
		else if (r == 0)
		{
			raise (SIGQUIT);
			return;
		}
		buf[r] = '\0';
		printf ("OOB : %s\n", buf);

		FD_ZERO (&xset);
		FD_SET (fd, &xset);
	}
	while (select (FD_SETSIZE, 0, 0, &xset, &tm) == 1);
}

int
main (int argc, char *argv[])
{
	struct sockaddr_in in;
	long r, pid;

	fd = socket (PF_INET, SOCK_STREAM, 0);
	if (fd < 0)
	{
		perror ("socket");
		return 1;
	}

	pid = getpid ();
	r = ioctl (fd, SIOCSPGRP, &pid);

#if !defined (OOBINLINE) && !defined (SELECT)
	signal (SIGURG, oob);
#else
	signal (SIGURG, SIG_IGN);
#ifdef OOBINLINE
	pid = 1;
	setsockopt (fd, SOL_SOCKET, SO_OOBINLINE, &pid, sizeof (pid));
#endif
#endif
	in.sin_family = AF_INET;
	in.sin_port = htons (5678);
	in.sin_addr.s_addr = argc < 2
		? htonl (INADDR_LOOPBACK)
		: inet_addr (argv[1]);

	r = connect (fd, (struct sockaddr *) &in, sizeof (in));
	if (r < 0)
	{
		perror ("connect");
		return 1;
	}

#ifdef SELECT
	while (1)
	{
		fd_set ex;

		FD_ZERO (&ex);
		FD_SET (fd, &ex);
		r = select (32, 0, 0, &ex, 0);
		if (r == 1 && FD_ISSET (fd, &ex))
			oob (0);
	}
#else
#ifndef OOBINLINE
	while (1)
		pause ();
#else
	{
		char buf[100];

		while (1)
		{
			r = read (fd, buf, sizeof (buf) - 1);
			if (r < 0)
			{
				perror ("read");
				return 1;
			}
			buf[r] = '\0';
			printf ("%s", buf);
			fflush (stdout);
		}
	}
#endif
#endif
	return 0;
}
