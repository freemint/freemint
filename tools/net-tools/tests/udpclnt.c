
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


# if __MINTLIB_MAJOR__ == 0 && __MINTLIB_MINOR__ < 57
typedef size_t socklen_t;
# endif

int
main (void)
{
	struct sockaddr_in in;
	socklen_t addrlen;
	int fd, r, i;

	in.sin_family = AF_INET;
	/* bind to the local address */	
	in.sin_addr.s_addr = htonl (INADDR_ANY);
	/* bind to an unused port */
	in.sin_port = htons (0);

	fd = socket (PF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
	{
		perror ("socket");
		return 0;
	}

	r = bind (fd, (struct sockaddr *) &in, sizeof (in));
	if (r < 0)
	{
		perror ("bind");
		return 0;
	}

	addrlen = sizeof (in);
	r = getsockname (fd, (struct sockaddr *) &in, &addrlen);
	if (r < 0)
	{
		perror ("getsockname");
		return 0;
	}

	printf ("My socket name is: \n\tport = %u \n\taddr = %x\n",
		in.sin_port,
		in.sin_addr.s_addr);

#ifdef DO_CHECKSUM
	/* turn on checksum generation */
	r = 1;
	r = setsockopt (fd, SOL_SOCKET, SO_CHKSUM, &r, sizeof (r));
	if (r < 0)
	{
		perror ("setsockopt");
		return 1;
	}
#endif
	in.sin_family = AF_INET;
	in.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
	in.sin_port = htons (5555);

	r = connect (fd, (struct sockaddr *) &in, sizeof (in));
	if (r < 0)
	{
		perror ("connect");
		return 0;
	}

#define FACT	400

	for (i = 1; i < 20; ++i)
	{
		static char buf[8000];

		r = write (fd, buf, i * FACT);
		if (r != i * FACT)
		{
			perror ("write");
			return 0;
		}
	}

	for (i = 1; i < 20; ++i)
	{
		static char buf[8000];

		r = read (fd, buf, sizeof (buf));
		if (r != i * FACT)
		{
			perror ("read");
			return 0;
		}
		printf ("Got Ack %d\n", i);
	}

	return 0;
}
