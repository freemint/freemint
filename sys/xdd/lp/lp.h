/*
 * Copyright 2002 Frank Naumann <fnaumann@freemint.de>
 * Copyright 1994 Thierry Bousch <bousch@suntopo.matups.fr>
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

# ifndef _lp_h
# define _lp_h

# include "mint/mint.h"
# include "libkern/libkern.h"


/*
 * Default settings. DEVNAME is the name of the device,
 * BUFSIZE is the size of the circular buffer,
 * in bytes. It should be an even long-number.
 */

# define DEVNAME	"u:\\dev\\lp"	/* device name     */
# define BUFSIZE	(24*1024L)	/* 24 kbyte buffer */


/*
 * Here are prototypes for the functions defined in "centr.s"
 */

void new_centr_vector (void);
void print_byte (int);


/* debug section
 */

# if 1
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


# endif /* _lp_h */
