/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * This file is free software; you can redistribute it and/or modify
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
 * 
 * begin:	2000-06-28
 * last change:	2000-06-28
 * 
 * Author:	Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# include "global.h"

# include "buf.h"
# include "sockdev.h"
# include "inet4/bpf.h"
# include "inet4/inet.h"
# include "unix/unix.h"

# include "version.h"

# include <mint/dcntl.h>
# include <mint/file.h>


# define MSG_VERSION	str (VER_MAJOR) "." str (VER_MINOR)
# define MSG_BUILDDATE	__DATE__

# define MSG_BOOT	\
	"\033p MiNT-Net " MSG_VERSION " PL " str (VER_PL) ", " VER_STATUS " \033q\r\n"

# define MSG_GREET	\
	"½ 1993-96 by Kay Roemer.\r\n" \
	"½ 1997-99 by Torsten Lang.\r\n" \
	"½ " MSG_BUILDDATE " by Frank Naumann.\r\n"

# define MSG_ALPHA	\
	"\033p WARNING: This is an unstable version - ALPHA! \033q\7\r\n"

# define MSG_BETA	\
	"\033p WARNING: This is a test version - BETA! \033q\7\r\n"

# define MSG_OLDMINT	\
	"\033pMiNT to old, this module requires at least a FreeMiNT 1.12!\033q\r\n"

# define MSG_FAILURE	\
	"\7Sorry, module NOT installed!\r\n\r\n"


static void (*init_func[])(void) =
{
	unix_init,
	inet_init,
	bpf_init,
	NULL
};

DEVDRV * init (struct kerinfo *k);

DEVDRV *
init (struct kerinfo *k)
{
	long r;
	
	KERNEL = k;
	
	c_conws (MSG_BOOT);
	c_conws (MSG_GREET);
# ifdef ALPHA
	c_conws (MSG_ALPHA);
# endif
# ifdef BETA
	c_conws (MSG_BETA);
# endif
	c_conws ("\r\n");
	
	if (!addroottimeout || !cancelroottimeout)
	{
		c_conws (MSG_OLDMINT);
		c_conws (MSG_FAILURE);
		
		return NULL;
	}
	
	if (buf_init () || init_sockdev ())
	{
		c_conws ("Cannot install socket device\n\r");
		c_conws (MSG_FAILURE);
		
		return NULL;
	}
	
	for (r = 0; init_func[r]; r++)
		(*init_func[r])();
	
	c_conws ("\r\n");
	return (DEVDRV *) 1;
}
