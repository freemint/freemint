/*  select.c -- MiNTLib.
    Copyright (C) 2000 Guido Flohr <guido@freemint.de>

    Modified to support Pure-C, Thorsten Otto.

    This file is part of the MiNTLib project, and may only be used
    modified and distributed under the terms of the MiNTLib project
    license, COPYMINT.  By continuing to use, modify, or distribute
    this file you indicate that you have read the license and
    understand and accept it fully.
*/

#ifdef __GNUC__
# define _GNU_SOURCE
#endif
#include "stsocket.h"
#include "mintsock.h"

/* Check the first NFDS descriptors each in READFDS (if not NULL) for read
   readiness, in WRITEFDS (if not NULL) for write readiness, and in EXCEPTFDS
   (if not NULL) for exceptional conditions.  If TIMEOUT is not NULL, time out
   after waiting the interval specified therein.  Returns the number of ready
   descriptors, or -1 for errors.  

   The function is currently emulated by means of poll().  This is 
   sub-optimal as long as NFDS is less or equal 32 because then we 
   waste quite some time by copying the file descriptor masks into
   struct poll.  Better poll will only be called when the native
   Fselect is not able to handle the call.  For the time being I want
   to test poll() and therefore ignore this problem.  
   
   If poll() has set POLLERR, POLLHUP, POLLNVAL or POLLMSG for any of the 
   polled descriptors, we simply mark that descriptor as ready for reading,
   writing or urgent reading so that the caller will get informed.  */
int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
	struct pollfd pfds[howmany(FD_SETSIZE, NFDBITS)];
	int i;
	long retval;
	long msec_timeout;
	int saved_errno = errno;
	
	if (nfds < 0 || nfds > FD_SETSIZE)
	{
		__set_errno(EINVAL);
		return -1;
#if 0
	} else if (nfds < 32)
	{
		/* Implement a native call to Fselect.  */
#endif
	}

	memset(pfds, 0, sizeof(pfds));

	/* Three loops are more efficient than one here.  */
	if (readfds != NULL)
		for (i = 0; i < nfds; i++)
			if (FD_ISSET(i, readfds))
			{
				pfds[i].fd = i;
				pfds[i].events |= POLLIN;
			}

	if (exceptfds != NULL)
		for (i = 0; i < nfds; i++)
			if (FD_ISSET(i, exceptfds))
			{
				pfds[i].fd = i;
				pfds[i].events |= POLLPRI;
			}

	if (writefds != NULL)
		for (i = 0; i < nfds; i++)
			if (FD_ISSET(i, writefds))
			{
				pfds[i].fd = i;
				pfds[i].events |= POLLOUT;
			}

	if (timeout == NULL)
	{
		msec_timeout = -1;
	} else
	{
		msec_timeout = timeout->tv_sec * 1000;
		msec_timeout += (timeout->tv_usec + 999) / 1000;
	}

	retval = poll(pfds, nfds, msec_timeout);

	if (retval < 0)
	{
		return (int)retval;
	} else
	{
		size_t sz = howmany(nfds, NFDBITS) * sizeof(fd_mask);

		if (readfds)
			memset(readfds, 0, sz);
		if (exceptfds)
			memset(exceptfds, 0, sz);
		if (writefds)
			memset(writefds, 0, sz);

		if (retval)
			for (i = 0; i < nfds; i++)
			{
				if (pfds[i].revents & (POLLIN | POLLRDNORM | POLLRDBAND))
					if (readfds != NULL)
						FD_SET(pfds[i].fd, readfds);
				if (pfds[i].revents & POLLPRI)
					if (exceptfds != NULL)
						FD_SET(pfds[i].fd, exceptfds);
				if (pfds[i].revents & (POLLOUT | POLLWRNORM | POLLWRBAND))
					if (writefds != NULL)
						FD_SET(pfds[i].fd, writefds);
				if (pfds[i].revents & (POLLERR | POLLNVAL | POLLHUP | POLLMSG))
				{
					if (readfds != NULL && FD_ISSET(pfds[i].fd, readfds))
						FD_SET(pfds[i].fd, readfds);
					if (exceptfds != NULL && FD_ISSET(pfds[i].fd, exceptfds))
						FD_SET(pfds[i].fd, exceptfds);
					if (writefds != NULL && FD_ISSET(pfds[i].fd, writefds))
						FD_SET(pfds[i].fd, writefds);
				}
			}
	}

	__set_errno(saved_errno);
	return (int)retval;
}
