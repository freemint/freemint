
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>


#define SENDING	10000000

static int pid = 0;

static void
sig_handler (int sig)
{
	printf ("%i: got signal %i\n", pid, sig);
	exit (0);
}

int
main (int argc, char *argv[])
{
	long bufsize;
	int fds[2];
	int ret;
	
	if (argc != 2)
	{
		printf ("you must spezify the buffersize as arg\n");
		return 0;
	}
	
	bufsize = atol (argv[1]);
	
	signal (SIGQUIT, sig_handler);
	signal (SIGINT, sig_handler);
	signal (SIGTERM, sig_handler);
	signal (SIGHUP, sig_handler);
	signal (SIGALRM, sig_handler);
	signal (SIGINT, sig_handler);
	signal (SIGPIPE, sig_handler);

	ret = pipe (fds);
	if (ret < 0)
	{
		perror ("pipe");
		return 0;
	}
	
	ret = fork ();
	if (ret < 0)
	{
		perror ("fork");
		goto out;
	}
	
	pid = getpid ();
	
	if (ret == 0)
	{
		/* child */
		
		char *buf;
		long nbytes;
		
		buf = malloc (bufsize);
		if (!buf)
		{
			perror ("malloc");
			goto out;
		}
		
		close (fds[0]);
		
		nbytes = 0;
		while (nbytes < SENDING)
		{
			ret = write (fds[1], buf, bufsize);
			if (ret < 0)
			{
				perror ("write");
				goto out;
			}
			else
				nbytes += ret;
		}
		
		close (fds[1]);
	}
	else
	{
		/* parent */
		
		clock_t start;
		clock_t end;
		long long bytes_per_second;
		long long nbytes;
		char *buf;
		
		buf = malloc (bufsize);
		if (!buf)
		{
			perror ("malloc");
			goto out;
		}
		
		close (fds[1]);
		
		nbytes = 0;
		start = clock ();
		do {
			ret = read (fds[0], buf, bufsize);
			if (ret < 0)
			{
				perror ("read");
				goto out;
			}
			nbytes += ret;
		}
		while (ret > 0);
		end = clock ();
		
		printf ("received %qd bytes (%qd kb) total in %li ticks\n",
			nbytes, nbytes / 1024, (long)(end - start));
		
		bytes_per_second = nbytes * CLOCKS_PER_SEC / (end - start);
		printf ("%qd bytes per second (%qd kb/s)\n",
			bytes_per_second, bytes_per_second / 1024);
		
		close (fds[0]);
	}
out:
	close (fds[0]);
	close (fds[1]);
	
	return 0;
}
