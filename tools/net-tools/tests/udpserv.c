
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int
main (void)
{
	struct sockaddr_in in;
	int fd, r;

	fd = socket (PF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
	{
		perror ("socket");
		return 0;
	}

	in.sin_family = AF_INET;
	in.sin_addr.s_addr = htonl (INADDR_ANY);
	in.sin_port = htons (5555);

	r = bind (fd, (struct sockaddr *) &in, sizeof (in));
	if (r < 0)
	{
		perror ("bind");
		return 0;
	}

	while (1)
	{
		socklen_t len = sizeof (in);
		static char buf[10000];

		r = recvfrom (fd, buf, sizeof (buf), 0,
			(struct sockaddr *) &in, &len);

		if (r < 0)
			continue;

		r = sendto (fd, buf, r, 0, (struct sockaddr *) &in,
			sizeof (in));
	}

	return 0;
}
