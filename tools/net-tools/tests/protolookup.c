
#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

int
main (int argc, char *argv[])
{
	struct protoent *pent;
	int i;

	for (i = 1; i < argc; ++i)
	{
		printf ("Looking up %s: ", argv[i]);
		fflush (stdout);

		pent = getprotobyname (argv[i]);
		if (!pent)
		{
			printf ("getprotobyname: proto not found\n");
			return 0;
		}

		printf ("ip protocol number %d\n", pent->p_proto);
	}

	return 0;
}
