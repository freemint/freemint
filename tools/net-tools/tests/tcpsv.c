
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>


#define BSIZE	8192

int
main (int argc, char *argv[])
{
	struct sockaddr_in in;
	int fd, nfd, r;
	static char buf[BSIZE];
	static char load[3 + (BSIZE + 1023) / 1024];
	long bytes, lap;

	memset (load, ' ', sizeof (load));
	load[0] = '\r';
	load[sizeof (load) - 1] = '<';

	fd = socket (PF_INET, SOCK_STREAM, 0);
	if (fd < 0)
	{
		perror ("socket");
		return 1;
	}

	in.sin_family = AF_INET;
	in.sin_port = htons (5555);
	in.sin_addr.s_addr = INADDR_ANY;
	if (bind (fd, (struct sockaddr *) &in, sizeof (in)) < 0)
	{
		perror ("bind");
		return 1;
	}

	if (listen (fd, 10) < 0)
	{
		perror ("listen");
		return 1;
	}

	nfd = accept (fd, 0, 0);
	if (nfd < 0)
	{
		perror ("accept");
		return 1;
	}

#if 0
	r = BSIZE;
	r = setsockopt (nfd, SOL_SOCKET, SO_RCVBUF, &r, sizeof (r));
	if (r < 0)
	{
		perror ("setsockopt");
		return 1;
	}
#endif

	bytes = 0;
	lap = time (0);
	do {
		r = read (nfd, buf, sizeof (buf));
		if (r < 0) {
			perror ("read");
			return 1;
		}
		bytes += r;
#if 0
		{
			long i;

			i = 1 + (r+1023)/1024;
			load[i] = '|';
			write (1, load, sizeof (load));
			load[i] = ' ';
		}
#endif
	} while (r > 0);

	printf ("\n%ld bytes per second\n", bytes / (time(0) - lap));
	
	if (close (nfd) < 0)
	{
		perror ("close(nfd)");
		return 1;
	}

	if (close (fd) < 0)
	{
		perror ("close(fd)");
		return 1;
	}

	return 0;
}
