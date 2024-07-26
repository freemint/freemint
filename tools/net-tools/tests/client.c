
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/types.h>

#define OFFSET	offsetof(struct sockaddr_un, sun_path)

int
main (void)
{
	struct sockaddr_un server_un;
	int fd, r;
	
	fd = socket (AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0)
	{
		perror ("socket");
		return 0;
	}
	server_un.sun_family = AF_UNIX;
	strcpy (server_un.sun_path, "/tmp/fort");

#ifdef NONBLOCK
	r = fcntl (fd, F_GETFL, 0);
	if (r < 0)
	{
		perror ("fcntl");
		return 0;
	}
	r |= O_NDELAY;
	r = fcntl (fd, F_SETFL, r);
	if (r < 0)
	{
		perror ("fcntl");
		return 0;
	}
#endif /* NONBLOCK */
	
	r = connect (fd, (struct sockaddr *)&server_un,
			OFFSET + strlen (server_un.sun_path));

#ifdef NONBLOCK
	if (0)
#else
	if (r < 0)
#endif
	{
		perror ("connect");
		close (fd);
		return 0;
	}
	else
	{
		socklen_t size;
		char buf[31];
		struct msghdr msg;
		struct iovec iov[3];
		long optval;
#ifdef NONBLOCK
		fd_set sel;

		FD_ZERO(&sel);
		FD_SET(fd, &sel);
		while (!select (32, NULL, &sel, NULL, NULL))
			FD_SET(fd, &sel);		
#endif
		size = sizeof (long);
		optval = 7000;
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

		iov[0].iov_base = &buf[0];
		iov[0].iov_len = 10;
		iov[1].iov_base = &buf[10];
		iov[1].iov_len = 10;
		iov[2].iov_base = &buf[20];
		iov[2].iov_len = 10;

		msg.msg_name = 0;
		msg.msg_namelen = 0;
		msg.msg_iov = iov;
		msg.msg_iovlen = 3;
		msg.msg_control = 0;
		msg.msg_controllen = 0;

		do {
			r = recvmsg (fd, &msg, 0);
			if (r < 0)
			{
				perror ("recvmsg");
				close (fd);
				return 0;
			}
			buf[r] = '\0';
			printf (buf);
		}
		while (r > 0);
	}

	close (fd);
	return 0;
}
