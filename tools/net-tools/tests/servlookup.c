
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

int
main (int argc, char *argv[])
{
	struct servent *sent;

	if (argc < 3)
	{
		printf ("Gimme more args!\n");
		return 0;
	}

	printf ("Looking up service %s/%s: ", argv[1], argv[2]);

	sent = getservbyname (argv[1], argv[2]);
	if (!sent)
	{
		printf ("getservbyname: lookup failure.\n");
		return 0;
	}

	printf ("port %d\n", ntohs (sent->s_port));
	return 0;
}
