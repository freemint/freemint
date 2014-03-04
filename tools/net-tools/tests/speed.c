
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>

# if __MINTLIB_MAJOR__ == 0 && __MINTLIB_MINOR__ < 57
typedef size_t socklen_t;
# endif

#define SERVER	"/tmp/fort"
#define OFFSET	((short)((struct sockaddr_un *) 0)->sun_path)

int
main (int argc, char *argv[])
{
	struct sockaddr_un server_un;
	void *buf;
	long bufsize;
	int fd, r;
	
	if (argc != 3)
	{
		printf ("give buffersize and rcv buffersize the args\n");
		return 0;
	}

	bufsize = atol (argv[1]);
	buf = malloc (bufsize);
	if (!buf)
	{
		printf ("out of mem");
		return 0;
	}

	fd = socket (AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0)
	{
		perror ("socket");
		return 0;
	}

	server_un.sun_family = AF_UNIX;
	strcpy (server_un.sun_path, SERVER);
	
	r = connect (fd, (struct sockaddr *) &server_un, OFFSET + strlen (SERVER));

	if (r < 0)
	{
		perror ("connect");
		close (fd);
		return 0;
	}
	else
	{
		socklen_t size;
		clock_t start;
		clock_t end;
		long optval;
		long long bytes_per_second;
		long long nbytes;

		size = sizeof (long);
		optval = atol (argv[2]);
		r = setsockopt (fd, SOL_SOCKET, SO_RCVBUF, &optval, size);
		if (r < 0)
		{
			perror ("setsockopt");
			return 0;
		}

		size = sizeof (long);
		r = getsockopt (fd, SOL_SOCKET, SO_RCVBUF, &optval, &size);
		if (r < 0)
		{
			perror ("getsockopt");
			return 0;
		}
		printf ("Rcv buffer: %ld bytes\n", optval);
		
		size = sizeof (long);
		r = getsockopt (fd, SOL_SOCKET, SO_SNDBUF, &optval, &size);
		if (r < 0)
		{
			perror ("getsockopt");
			return 0;
		}

		printf ("Snd buffer: %ld bytes\n\n", optval);

		nbytes = 0;
		start = clock ();
		do {
			r = read (fd, buf, bufsize);
			if (r < 0)
			{
				perror ("read");
				close (fd);
				return 0;
			}
			nbytes += r;
		}
		while (r > 0);
		end = clock ();

		printf ("received %qd bytes (%qd kb) total in %li ticks\n",
			nbytes, nbytes / 1024, (long)(end - start));

		bytes_per_second = nbytes * CLOCKS_PER_SEC / (end - start);
		printf ("%qd bytes per second (%qd kb/s)\n",
			bytes_per_second, bytes_per_second / 1024);
	}

	close (fd);
	return 0;
}
