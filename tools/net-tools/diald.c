/*
 * dial interface daemon, (w) 1999 Torsten Lang.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sockios.h>
#include <errno.h>


#define DIAL_CHANNELS 4


int
main (void)
{
	char dial_skript[]   = "/etc/ppp/ppp-up0";
	char hangup_skript[] = "/etc/ppp/ppp-down0";
	char dialdev_name[]  = "/dev/dial0";
	char op;
	int i, fd;
	fd_set fds;

	/* spawn DIAL_CHANNELS child processes */
	for (i = 0; i < DIAL_CHANNELS; i++)
		if (!fork ())
			break;

	/* parent exits here */
	if (i == DIAL_CHANNELS)
		return 0;

	/* children come here */
	dial_skript[sizeof(dial_skript)-2]     += i;
	hangup_skript[sizeof(hangup_skript)-2] += i;
	dialdev_name[sizeof(dialdev_name)-2]   += i;

	fd = open (dialdev_name, O_RDONLY);
	if (fd > 0)
	{
		FD_ZERO (&fds);
		FD_SET (fd, &fds);
		while (1)
		{
			select (1, &fds, 0, 0, NULL);
			if (read (fd, &op, 1))
			{
				if (op & 0x80)
					system (hangup_skript);
				else
					system (dial_skript);
			}
		}
	}

	printf ("diald error: device %s could not be opened!\n", dialdev_name);
	return EACCES;
}
