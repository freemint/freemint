
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>


#define SENDING	10000000

#define SERVER	"/tmp/fort"
#define OFFSET	((short)((struct sockaddr_un *) 0)->sun_path)

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
main (int argc, char *argv[])
{
	int err;
	void *buf;
	long bufsize;
	struct sockaddr_un un;

	if (argc != 2)
	{
		printf ("you must spezify the buffersize as arg\n");
		return 0;
	}

	bufsize = atol (argv[1]);
	buf = malloc (bufsize);
	if (!buf)
	{
		printf ("out of mem");
		return 0;
	}
	
	printf ("Send bufsize is %ld\n", bufsize);
	memset (buf, 'A', bufsize);
	
	fd = socket (AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0)
	{
		perror ("socket");
		return 0;
	}

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
		int client;
		long nbytes;
		long r;

		client = accept (fd, NULL, NULL);
		if (client < 0)
		{
			perror ("accept");
			break;
		}

		nbytes = 0;
		while (nbytes < SENDING)
		{
			r = write (client, buf, bufsize);
			if (r < 0)
				perror ("write");
			else
				nbytes += r;
		}

		close (client);
	}

	close (fd);
	unlink (SERVER);
	return 0;
}
