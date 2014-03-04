
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>

# if __MINTLIB_MAJOR__ == 0 && __MINTLIB_MINOR__ < 57
typedef size_t socklen_t;
# endif

#define SENDING	10000000

#define SERVER	"/tmp/fort"
#define OFFSET	((short)((struct sockaddr_un *) 0)->sun_path)

static int run = 1;
static int fd = 0;

static void
sig_handler (int sig)
{
	close (fd);
	unlink (SERVER);
	exit (0);
}

int
main (int argc, char *argv[])
{
	long bufsize;
	int r;
	
	if (argc != 3)
	{
		printf ("give buffersize and rcv buffersize as arguments\n");
		return 0;
	}

	bufsize = atol (argv[1]);
	printf ("bufsize is %ld\n", bufsize);

	r = fork ();
	if (r < 0)
	{
		perror ("fork");
		return 0;
	}
	
	if (r == 0)
	{
		/* child */
		
		struct sockaddr_un un;
		void *buf;
		
		buf = malloc (bufsize);
		if (!buf)
		{
			printf ("out of memory");
			close (fd);
			return 0;
		}
		memset (buf, 'A', bufsize);
		
		fd = socket (AF_UNIX, SOCK_STREAM, 0);
		if (fd < 0)
		{
			perror ("socket");
			return 0;
		}
		
		un.sun_family = AF_UNIX;
		strcpy (un.sun_path, SERVER);
		
		r = bind (fd, (struct sockaddr *) &un, OFFSET + strlen (un.sun_path));
		if (r < 0)
		{
			perror ("bind");
			close (fd);
			return 0;
		}
		
		r = listen (fd, 2);
		if (r < 0)
		{
			perror ("listen");
			close (fd);
			return 0;
		}	
		
		signal (SIGQUIT, sig_handler);
		signal (SIGINT, sig_handler);
		signal (SIGTERM, sig_handler);
		signal (SIGHUP, sig_handler);
		
		// while (run)
		{
			int client;
			long nbytes;
			
			client = accept (fd, NULL, NULL);
			if (client < 0)
			{
				perror ("accept");
				// break;
			}
			
			nbytes = 0;
			while (nbytes < SENDING)
			{
				r = write (client, buf, bufsize);
				if (r < 0)
					perror ("write");
				else
					nbytes += r;
			}
			
			close (client);
		}
		
		unlink (SERVER);
	}
	else
	{
		/* parent */
		
		struct sockaddr_un un;
		void *buf;
		
		buf = malloc (bufsize);
		if (!buf)
		{
			printf ("out of memory");
			close (fd);
			return 0;
		}
		
		/* schedule */
		sleep (2);
		
		fd = socket (AF_UNIX, SOCK_STREAM, 0);
		if (fd < 0)
		{
			perror ("socket");
			return 0;
		}
		
		un.sun_family = AF_UNIX;
		strcpy (un.sun_path, SERVER);
		
		r = connect (fd, (struct sockaddr *) &un, OFFSET + strlen (un.sun_path));
		if (r < 0)
		{
			perror ("connect");
			close (fd);
			return 0;
		}
		else
		{
			socklen_t size;
			clock_t start;
			clock_t end;
			long optval;
			long long bytes_per_second;
			long long nbytes;
			
			size = sizeof (long);
			optval = atol (argv[2]);
			r = setsockopt (fd, SOL_SOCKET, SO_RCVBUF, &optval, size);
			if (r < 0)
			{
				perror ("setsockopt");
				return 0;
			}
			
			size = sizeof (long);
			r = getsockopt (fd, SOL_SOCKET, SO_RCVBUF, &optval, &size);
			if (r < 0)
			{
				perror ("getsockopt");
				return 0;
			}
			printf ("Rcv buffer: %ld bytes\n", optval);
			
			size = sizeof (long);
			r = getsockopt (fd, SOL_SOCKET, SO_SNDBUF, &optval, &size);
			if (r < 0)
			{
				perror ("getsockopt");
				return 0;
			}
			printf ("Snd buffer: %ld bytes\n\n", optval);
			
			nbytes = 0;
			start = clock ();
			do {
				r = read (fd, buf, bufsize);
				if (r < 0)
				{
					perror ("read");
					close (fd);
					return 0;
				}
				nbytes += r;
			}
			while (r > 0);
			end = clock ();
			
			printf ("received %qd bytes (%qd kb) total in %li ticks\n",
				nbytes, nbytes / 1024, (long)(end - start));
			
			bytes_per_second = nbytes * CLOCKS_PER_SEC / (end - start);
			printf ("%qd bytes per second (%qd kb/s)\n",
				bytes_per_second, bytes_per_second / 1024);
		}
	}
	
	close (fd);
	return 0;
}
