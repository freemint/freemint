/*  poll.c -- MiNTLib.
    Copyright (C) 2000 Guido Flohr <guido@freemint.de>

    Modified to support Pure-C, Thorsten Otto.

    This file is part of the MiNTLib project, and may only be used
    modified and distributed under the terms of the MiNTLib project
    license, COPYMINT.	By continuing to use, modify, or distribute
    this file you indicate that you have read the license and
    understand and accept it fully.
*/

#ifdef __GNUC__
#define _GNU_SOURCE /* for POLLRDNORM */
#endif
#include "stsocket.h"
#include "mintsock.h"
#ifdef __PUREC__
# include <tos.h>
long Fpoll(void *fds, long nfds, unsigned long timeout);
#else
# include <mint/mintbind.h>
#endif


int poll(struct pollfd *fds, unsigned long int nfds, int32_t __timeout)
{
	long int retval;
	unsigned long timeout = (unsigned long) __timeout;

	if (__timeout < 0)
	{
		timeout = ~0;
	}

	retval = Fpoll(fds, nfds, timeout);
	if (retval != -ENOSYS)
	{
		if (retval < 0)
		{
			__set_errno(-(int)retval);
			return -1;
		}
	} else
	{
		/* We must emulate the call via Fselect ().  First task is to
		   set up the file descriptor masks.    */
		long rfds = 0;
		long wfds = 0;
		long xfds = 0;
		unsigned long int i;
		struct pollfd *pfds = fds;

		for (i = 0; i < nfds; i++)
		{
			pfds[i].revents = 0;

			/* Older than 1.19 can't do more than 32 file descriptors.
			 * And we'd only get here if we're a very old kernel anyway.
			 */
			if (pfds[i].fd >= 32)
			{
				pfds[i].revents = POLLNVAL;
				continue;
			}
#define LEGAL_FLAGS (POLLIN | POLLPRI | POLLOUT | POLLRDNORM | POLLWRNORM | POLLRDBAND | POLLWRBAND)

			if ((pfds[i].events | LEGAL_FLAGS) != LEGAL_FLAGS)
			{
				pfds[i].revents = POLLNVAL;
				continue;
			}

			if (pfds[i].events & (POLLIN | POLLRDNORM))
				rfds |= (1L << (pfds[i].fd));
			if (pfds[i].events & POLLPRI)
				xfds |= (1L << (pfds[i].fd));
			if (pfds[i].events & (POLLOUT | POLLWRNORM))
				wfds |= (1L << (pfds[i].fd));
		}

		if (__timeout < 0)
		{
			retval = Fselect(0, &rfds, &wfds, &xfds);
		} else if (timeout == 0)
		{
			retval = Fselect(1, &rfds, &wfds, &xfds);
		} else if (timeout < USHRT_MAX)
		{
			/* The manpage Fselect(2) says that timeout is
			   signed.  But it is really unsigned.  */
			retval = Fselect((unsigned short)timeout, &rfds, &wfds, &xfds);
		} else
		{
			/* Thanks to the former kernel hackers we have
			   to loop in order to simulate longer timeouts
			   than USHRT_MAX.  */
			unsigned long saved_rfds;
			unsigned long saved_wfds;
			unsigned long saved_xfds;
			unsigned short int this_timeout;

			saved_rfds = rfds;
			saved_wfds = wfds;
			saved_xfds = xfds;

			for (;;)
			{
				if (timeout > USHRT_MAX)
					this_timeout = USHRT_MAX;
				else
					this_timeout = timeout;

				retval = Fselect(this_timeout, &rfds, &wfds, &xfds);

				if (retval != 0)
					break;

				timeout -= this_timeout;
				if (timeout == 0)
					break;

				/* I don't know whether we can rely on the
				   masks not being clobbered on timeout.  */
				rfds = saved_rfds;
				wfds = saved_wfds;
				xfds = saved_xfds;
			}
		}

		/* Now fill in the results in struct pollfd.    */
		for (i = 0; i < nfds; i++)
		{
			/* Older than 1.19 can't do more than 32 file descriptors. */
			if (pfds[i].fd >= 32)
				continue;
			if (rfds & (1L << (pfds[i].fd)))
				pfds[i].revents |= (pfds[i].events & (POLLIN | POLLRDNORM));
			if (wfds & (1L << (pfds[i].fd)))
				pfds[i].revents |= (pfds[i].events & (POLLOUT | POLLWRNORM));
			if (xfds & (1L << (pfds[i].fd)))
				pfds[i].revents |= (pfds[i].events & POLLPRI);
		}

		if (retval < 0)
		{
			__set_errno(-(int)retval);
			return -1;
		}
	}

	return (int)retval;
}
