
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>


#define NONBLOCK

#define SERVER	"/tmp/fort"
#define OFFSET ((short)((struct sockaddr_un *)0)->sun_path)

static int run = 1;
static int fd = 0;

static void
sig_handler (int sig)
{
	close (fd);
	unlink (SERVER);
	exit (0);
}

int
main (void)
{
	int err, r;
	struct sockaddr_un un;

	fd = socket (AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0)
	{
		perror ("socket");
		return 0;
	}

#ifdef NONBLOCK
	r = fcntl (fd, F_GETFL, 0);
	if (r < 0)
	{
		perror ("fcntl");
		return 0;
	}

	r |= O_NDELAY;
	r = fcntl (fd, F_SETFL, r);
	if (r < 0)
	{
		perror ("fcntl");
		return 0;
	}
#endif /* NONBLOCK */

	un.sun_family = AF_UNIX;
	strcpy (un.sun_path, SERVER);

	err = bind (fd, (struct sockaddr *) &un, OFFSET+strlen (un.sun_path));
	if (err < 0)
	{
		perror ("bind");
		close (fd);
		return 0;
	}

	err = listen (fd, 2);
	if (err < 0)
	{
		perror ("listen");
		close (fd);
		return 0;
	}	

	signal (SIGQUIT, sig_handler);
	signal (SIGINT, sig_handler);
	signal (SIGTERM, sig_handler);
	signal (SIGHUP, sig_handler);

	while (run)
	{
		int client, i;
		char message1[] = "When the ";
		char message2[] = "world is ";
		char message3[] = "running down...\n";
		struct msghdr msg;
		struct iovec iov[3];
		
#ifdef NONBLOCK
		fd_set sel;

		FD_ZERO(&sel);
		FD_SET(fd, &sel);

		while (!select (32, &sel, NULL, NULL, NULL))
			FD_SET(fd, &sel);
#endif		
		client = accept (fd, NULL, NULL);
		if (client < 0)
		{
			perror ("accept");
			break;
		}

		iov[0].iov_base = message1;
		iov[0].iov_len  = strlen (message1);
		iov[1].iov_base = message2;
		iov[1].iov_len  = strlen (message2);
		iov[2].iov_base = message3;
		iov[2].iov_len  = strlen (message3);

		msg.msg_name = 0;
		msg.msg_namelen = 0;
		msg.msg_iov = iov;
		msg.msg_iovlen = 3;
		msg.msg_accrights = 0;
		msg.msg_accrightslen = 0;

		for (i=0; i<1000; ++i)
		{
			long size = iov[0].iov_len + iov[1].iov_len +
				iov[2].iov_len;

			if (size != sendmsg (client, &msg, 0))
			{
				perror ("sendmsg");
				break;
			}
		}

		close (client);
	}

	close (fd);
	unlink (SERVER);
	return 0;
}
