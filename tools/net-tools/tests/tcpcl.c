
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>


#define BSIZE	8192

int
main (int argc, char *argv[])
{
	struct sockaddr_in in;
	int fd, r;
	static char buf[1500];

	fd = socket (PF_INET, SOCK_STREAM, 0);
	if (fd < 0)
	{
		perror ("socket");
		return 1;
	}

	in.sin_family = AF_INET;
	in.sin_port = 6666;
	in.sin_addr.s_addr = INADDR_ANY;
	if (bind (fd, (struct sockaddr *) &in, sizeof (in)) < 0)
	{
		perror ("bind");
		return 1;
	}

	r = BSIZE;
	setsockopt (fd, SOL_SOCKET, SO_SNDBUF, &r, sizeof (r));
	
	in.sin_family = AF_INET;
	in.sin_port = htons (5555);
	in.sin_addr.s_addr = argc < 2
		? htonl (INADDR_LOOPBACK)
		: inet_addr (argv[1]);

	if (connect (fd, (struct sockaddr *) &in, sizeof (in)) < 0)
	{
		perror ("connect");
		return 1;
	}

	do {
		r = write (fd, buf, sizeof (buf));
		if (r < 0)
		{
			perror ("write");
			return 1;
		}
	}
	while (/*--n &&*/ r > 0);

	if (close (fd) < 0)
	{
		perror ("close");
		return 1;
	}

	return 0;
}
