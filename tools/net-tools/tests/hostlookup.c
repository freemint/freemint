/*
 * Get the IP addresses of the hosts given on the command line
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <arpa/nameser.h>
#include <resolv.h>

int
main (int argc, char *argv[])
{
	struct hostent *hent;
	int i;

	_res.options |= RES_DEBUG;

	for (i = 1; i < argc; ++i)
	{
		printf ("Looking up %s: ", argv[i]);
		fflush (stdout);

		hent = gethostbyname (argv[i]);
		if (!hent)
		{
			herror ("gethostbyname");
			perror ("gethostbyname");
			return 0;
		}

		puts (inet_ntoa (*(struct in_addr *)hent->h_addr));
	}

	return 0;
}
