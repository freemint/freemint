/*
 *	masquerading(8) utility for MintNet, (w) 1999, Mario Becroft.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "masquerade.h"


static char USAGE_TEXT [] =
"Usage: %s [command parameters] [command parameters...]\n"
"\n"
"Any number of commands and associated parameters may be specified.\n"
"Invoke with no arguments to show current configuration.\n"
"\n"
"Commands:\n"
"  address:            Set the address of network to be masqueraded for\n"
"  netmask:            Set the netmask of network to be masqueraded for\n"
"  flags:              Set flags to specified decimal value\n"
"  set:                Set a specified flag\n"
"  unset:              Clear a specified flag\n"
"  redirect:           Redirect specified local port to specified address and port\n"
"  unredirect:         Stop redirecting specified local port\n"
"  purge:              Purge all condemned (timed out) database entries\n"
"  tcp-first-timeout,\n"
"  tcp-ack-timeout,\n"
"  tcp-fin-timeout,\n"
"  udp-timeout\n"
"  and icmp-timeout:   Set the timeout for specified protocol\n"
"\n"
"Flags:\n"
"  ENABLED        0x01 If set, IP masquerading is enabled\n"
"  LOCAL_PACKETS  0x02 If set, packets destined for the local host will be\n"
"                      masqueraded. Useful only for debugging\n"
"  DEFRAG_ALL     0x04 If set, all packets will be defragmented before further\n"
"                      processing. Should normally be set.\n"
;


static int
set_long (int pos, unsigned long value, const char *progname)
{
	int fd;
	
	fd = open ("/dev/masquerade", O_WRONLY);
	if (fd > 0)
	{
		lseek (fd, pos, SEEK_SET);
		if (write (fd, &value, 4) != 4)
			fprintf(stderr, "%s: failed to set parameter.\n", progname);

		close (fd);
	}
	else
	{
		fprintf (stderr, "%s: could not open /dev/masquerade.\n", progname);
		return 0;
	}

	return 1;
}

static void
display_config (char *program_name)
{
	int fd, i;
	struct in_addr addr;
	char *ascii_addr;
	struct protoent *pent;
	unsigned long val;
	PORT_DB_RECORD record;
	unsigned long cur_time;
	
	fd = open("/dev/masquerade", 0);
	if (fd >= 0)
	{
		printf ("Masquerading for:\n\n");

		lseek (fd, 2, SEEK_SET);
		read (fd, &val, sizeof (val));
		addr.s_addr = val;
		ascii_addr = inet_ntoa (addr);
		printf ("Address: %s\n", ascii_addr);

		lseek (fd, 3, SEEK_SET);
		read (fd, &val, sizeof (val));
		addr.s_addr = val;
		ascii_addr = inet_ntoa (addr);
		printf ("Netmask: %s\n", ascii_addr);
		
		lseek (fd, 4, SEEK_SET);
		read(fd, &val, sizeof(val));
		printf ("Flags: %lu (0x%lX) = %s%s%s.\n", val, val,
			val & MASQ_ENABLED ? "ENABLED " : "",
			val & MASQ_LOCAL_PACKETS ? "LOCAL_PACKETS " : "",
			val & MASQ_DEFRAG_ALL ? "DEFRAG_ALL " : ""
		);
		
		printf ("\n");
		printf ("Timeouts: ");

		lseek (fd, 5, SEEK_SET);
		read (fd, &val, sizeof (val));
		printf ("tcp-first %lu ", val);

		lseek (fd, 6, SEEK_SET);
		read (fd, &val, sizeof (val));
		printf("tcp-ack %lu ", val);

		lseek (fd, 7, SEEK_SET);
		read (fd, &val, sizeof (val));
		printf ("tcp-fin %lu ", val);

		lseek (fd, 8, SEEK_SET);
		read (fd, &val, sizeof (val));
		printf ("udp %lu ", val);

		lseek (fd, 9, SEEK_SET);
		read (fd, &val, sizeof (val));
		printf ("icmp %lu.\n", val);
		
		lseek (fd, 50, SEEK_SET);
		read (fd, &cur_time, sizeof (cur_time));

		printf ("\n");
		printf ("Masqueraded connections:\n");
		printf ("G.port\tAddress\t\tPort\tProto\tModified\tTimeout\n");

		lseek (fd, 100, SEEK_SET);
		i = read (fd, &record, sizeof (record));
		while (i == sizeof (record))
		{
			addr.s_addr = record.masq_addr;
			ascii_addr = inet_ntoa (addr);
			pent = getprotobynumber (record.proto);
			printf ("%d\t%s\t%ld\t%s\t%ld\t%ld\t%s\n", MASQ_BASE_PORT+record.num, ascii_addr, (unsigned long) record.masq_port, pent->p_name, record.modified, record.timeout, record.modified + record.timeout < cur_time ? "CONDEMNED" : "");
			i = read (fd, &record, sizeof (record));
		}
		
		printf ("\n");
		printf ("Redirected ports:\n");
		printf ("G.port\tAddress\t\tPort\n");

		lseek (fd, 200, SEEK_SET);
		i = read (fd, &record, sizeof (record));
		while (i == sizeof (record))
		{
			addr.s_addr = record.masq_addr;
			ascii_addr = inet_ntoa (addr);
			printf ("%ld\t%s\t%ld\n", (unsigned long) record.num, ascii_addr, (unsigned long) record.masq_port);
			i = read (fd, &record, sizeof(record));
		}
		
		close (fd);
	}
	else
		fprintf (stderr, "%s: could not open /dev/masquerade.\n", program_name);
}

int
main (int argc, char *argv[])
{
	if (argc == 1)
		display_config (argv[0]);
	else
	{
		int i, fd, show_usage = 0;
		struct in_addr addr;
		unsigned short port, masq_port;
		unsigned long val;
		PORT_DB_RECORD record;
		
		i = 1;
		
		while (i < argc)
		{
			if ((strcmp (argv[i], "address") == 0)  && argc > i + 1)
			{
				inet_aton (argv[++i], &addr);
				printf ("Setting address to %s.\n", inet_ntoa (addr));
				set_long (2, addr.s_addr, argv[0]);
			}
			else if ((strcmp(argv[i], "netmask") == 0) && argc > i + 1)
			{
				inet_aton (argv[++i], &addr);
				printf ("Setting netmask to %s.\n", inet_ntoa (addr));
				set_long (3, addr.s_addr, argv[0]);
			}
			else if ((strcmp (argv[i], "flags") == 0) && argc > i + 1)
			{
				val = atol (argv[++i]);
				printf ("Setting flags to %lu (0x%lX).\n", val, val);
				set_long (4, val, argv[0]);
			}
			else if ((strcmp (argv[i], "set") == 0) && argc > i + 1)
			{
				fd = open ("/dev/masquerade", O_RDWR);
				if (fd > 0)
				{
					lseek (fd, 4, SEEK_SET);
					read (fd, &val, sizeof(val));
					
					if (strcmp (argv[++i], "ENABLED") == 0)
						val |= MASQ_ENABLED;
					else if (strcmp (argv[i], "LOCAL_PACKETS") == 0)
						val |= MASQ_LOCAL_PACKETS;
					else if (strcmp (argv[i], "DEFRAG_ALL") == 0)
						val |= MASQ_DEFRAG_ALL;
					
					printf ("Setting flags to %lu (0x%lX).\n", val, val);
					
					lseek (fd, 4, SEEK_SET);
					if (write (fd, &val, 4) != 4)
						fprintf (stderr, "%s: could not set flags.\n", argv[0]);

					close (fd);
				}
				else
					fprintf (stderr, "%s: could not open /dev/masquerade.\n", argv[0]);
			}
			else if ((strcmp (argv[i], "unset") == 0) && argc > i + 1)
			{
				fd = open ("/dev/masquerade", O_RDWR);
				if (fd > 0)
				{
					lseek (fd, 4, SEEK_SET);
					read (fd, &val, sizeof(val));
					
					if (strcmp (argv[++i], "ENABLED") == 0)
						val &= ~MASQ_ENABLED;
					else if (strcmp (argv[i], "LOCAL_PACKETS") == 0)
						val &= ~MASQ_LOCAL_PACKETS;
					else if (strcmp (argv[i], "DEFRAG_ALL") == 0)
						val &= ~MASQ_DEFRAG_ALL;
					
					printf ("Unsetting flags to %lu (0x%lX).\n", val, val);
					
					lseek (fd, 4, SEEK_SET);
					if (write (fd, &val, 4) != 4)
						fprintf (stderr, "%s: could not set flags.\n", argv[0]);

					close (fd);
				}
				else
					fprintf (stderr, "%s: could not open /dev/masquerade.\n", argv[0]);
			}
			else if ((strcmp (argv[i], "redirect") == 0) && argc > i + 3)
			{
				port = atoi (argv[++i]);
				inet_aton (argv[++i], &addr);
				masq_port = atoi (argv[++i]);
				printf ("Adding redirection for port %ld to host %s port %ld.\n", (long)port, inet_ntoa(addr), (long)masq_port);
				fd = open ("/dev/masquerade", O_WRONLY);
				if (fd > 0)
				{
					record.num = port;
					record.masq_addr = addr.s_addr;
					record.masq_port = masq_port;
					record.dst_port = 0;
					record.proto = 255;
					record.modified = 0;
					record.timeout = 0;
					record.offs = 0;
					record.prev_offs = 0;
					record.seq = 0;
					record.next_port = NULL;
					lseek(fd, 200, SEEK_SET);
					if (write (fd, &record, sizeof (record)) != sizeof(record))
						fprintf (stderr, "%s: could not add redirection.\n", argv[0]);

					close (fd);
				}
				else
					fprintf(stderr, "%s: could not open /dev/masquerade.\n", argv[0]);
			}
			else if ((strcmp (argv[i], "unredirect") == 0) && argc > i + 1)
			{
				port = atoi(argv[++i]);
				printf ("Removing redirection for port %ld.\n", (long)port);
				fd = open ("/dev/masquerade", O_RDWR);
				if (fd > 0)
				{
					lseek (fd, 200, SEEK_SET);
					i = read (fd, &record, sizeof(record));
					while (i == sizeof (record))
					{
						if (record.num == port)
							break;
						i = read (fd, &record, sizeof(record));
					}
					
					if (i != sizeof (record))
					{
						fprintf (stderr, "%s: redirection does not exist, and cannot be removed.\n", argv[0]);
					}
					else
					{
						lseek (fd, 201, SEEK_SET);
						if (write (fd, &port, sizeof(port)) != sizeof(port))
							fprintf (stderr, "%s: could not remove redirection.\n", argv[0]);
					}
					close (fd);
				}
				else
					fprintf (stderr, "%s: could not open /dev/masquerade.\n", argv[0]);
			}
			else if ((strcmp (argv[i], "purge") == 0))
			{
				printf ("Purging all timed out records.\n");
				fd = open ("/dev/masquerade", O_WRONLY);
				if (fd > 0)
				{
					lseek(fd, 102, SEEK_SET);
					if (write (fd, "purge", 5) != 5)
						fprintf(stderr, "%s: could not purge records.\n", argv[0]);

					close (fd);
				}
				else
					fprintf (stderr, "%s: could not open /dev/masquerade.\n", argv[0]);
			}
			else if ((strcmp (argv[i], "tcp-first-timeout") == 0) && argc > i + 1)
			{
				val = atol (argv[++i]);
				printf ("Setting tcp-first timeout to %lu.\n", val);
				set_long (5, val, argv[0]);
			}
			else if ((strcmp (argv[i], "tcp-ack-timeout") == 0) && argc > i + 1)
			{
				val = atol (argv[++i]);
				printf ("Setting tcp-ack timeout to %lu.\n", val);
				set_long (6, val, argv[0]);
			}
			else if ((strcmp( argv[i], "tcp-fin-timeout") == 0) && argc > i + 1)
			{
				val = atol (argv[++i]);
				printf ("Setting tcp-fin timeout to %lu.\n", val);
				set_long (7, val, argv[0]);
			}
			else if ((strcmp (argv[i], "udp-timeout") == 0) && argc > i + 1)
			{
				val = atol (argv[++i]);
				printf ("Setting udp timeout to %lu.\n", val);
				set_long (8, val, argv[0]);
			}
			else if ((strcmp (argv[i], "icmp-timeout") == 0) && argc > i + 1)
			{
				val = atol (argv[++i]);
				printf ("Setting icmp timeout to %lu.\n", val);
				set_long (9, val, argv[0]);
			}
			else
				show_usage = 1;
			
			i++;
		}

		if (show_usage)
			printf (USAGE_TEXT, argv[0]);
	}
	
	return 0;
}
