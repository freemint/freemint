
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

int
main (int argc, char *argv[])
{
	struct sockaddr_in in;
	int fd, clfd, r;
	static char buf[10];

	fd = socket (PF_INET, SOCK_STREAM, 0);
	if (fd < 0)
	{
		perror ("socket");
		return 1;
	}

	in.sin_family = AF_INET;
	in.sin_port = htons (5678);
	in.sin_addr.s_addr = INADDR_ANY;
	r = bind (fd, (struct sockaddr *) &in, sizeof (in));
	if (r < 0)
	{
		perror ("bind");
		return 1;
	}
	listen (fd, 10);
	while (1)
	{
		clfd = accept (fd, 0, 0);
		if (clfd < 0)
		{
			perror ("accept");
			return 1;
		}

		memset (buf, 'A', sizeof (buf));
		write (clfd, buf, sizeof (buf));

		for (r = 0; r < 3; buf[r] = '1' + r, ++r)
			;

		send (clfd, buf, 3, MSG_OOB);
		/* sleep (1); */

		memset (buf, 'B', sizeof (buf));
		write (clfd, buf, sizeof (buf));

		for (r = 0; r < 3; buf[r] = '3' + r, ++r)
			;

		send (clfd, buf, 3, MSG_OOB);

		close (clfd);
	}
	
	return 0;
}
