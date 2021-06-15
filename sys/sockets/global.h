/*
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 */

# ifndef _global_h
# define _global_h

# include "mint/mint.h"
# include "libkern/libkern.h"

/* own default header */
# include "config.h"


# define isleep		sleep


/* debug section
 */

# if 0
# define DEV_DEBUG	1
# endif

# ifdef DEV_DEBUG

# define FORCE(x)	
# define ALERT(x)	KERNEL_ALERT x
# define DEBUG(x)	KERNEL_DEBUG x
# define TRACE(x)	KERNEL_TRACE x
# define ASSERT(x)	assert x

# else

# define FORCE(x)	
# define ALERT(x)	KERNEL_ALERT x
# define DEBUG(x)	
# define TRACE(x)	
# define ASSERT(x)	assert x

# endif


/* XXX hack alert
 * must be in kerinterface or in kerheaders
 * (redundant with freemint/sys/ipc_socketutil.h)
 * 
 * so_rselect(), so_wselect(), so_wakersel(), so_wakewsel() handle
 * processes for selecting.
 */

# include "mint/net.h"

static inline long
so_rselect (struct socket *so, long proc)
{
	if (so->rsel)
		return 2;
	
	so->rsel = proc;
	return 0;
}

static inline long
so_wselect (struct socket *so, long proc)
{
	if (so->wsel)
		return 2;
	
	so->wsel = proc;
	return 0;
}

static inline long
so_xselect (struct socket *so, long proc)
{
	if (so->xsel)
		return 2;
	
	so->xsel = proc;
	return 0;
}

INLINE void
so_wakersel (struct socket *so)
{
	if (so->rsel)
		wakeselect (so->rsel);
}

static inline void
so_wakewsel (struct socket *so)
{
	if (so->wsel)
		wakeselect (so->wsel);
}

static inline void
so_wakexsel (struct socket *so)
{
	if (so->xsel)
		wakeselect (so->xsel);
}


# endif /* _global_h */
