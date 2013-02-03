/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000, 2001 Frank Naumann <fnaumann@freemint.de>
 * Copyright 1997, 1998, 1999 Torsten Lang
 * Copyright 1993, 1994, 1995, 1996 Kay Roemer
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
# include "inet4/init.h"

# include "version.h"

# include "mint/dcntl.h"
# include "mint/file.h"


# define MSG_VERSION	str (VER_MAJOR) "." str (VER_MINOR)
# define MSG_BUILDDATE	__DATE__

# define MSG_BOOT	\
	"\033p MiNT-Net TCP/IP " MSG_VERSION " PL " str (VER_PL) ", " VER_STATUS " \033q\r\n"

# define MSG_GREET	\
	" 1993-96 by Kay Roemer.\r\n" \
	" 1997-99 by Torsten Lang.\r\n" \
	" " MSG_BUILDDATE " by Frank Naumann.\r\n"

# define MSG_ALPHA	\
	"\033p WARNING: This is an unstable version - ALPHA! \033q\7\r\n"

# define MSG_BETA	\
	"\033p WARNING: This is a test version - BETA! \033q\7\r\n"

# define MSG_OLDMINT	\
	"\033pMiNT is not the correct version, this module requires FreeMiNT 1.18!\033q\r\n"

# define MSG_FAILURE	\
	"\7Sorry, module NOT installed!\r\n\r\n"


static void (*init_func[])(void) =
{
	inet4_init,
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
	
	if (MINT_MAJOR != 1 || MINT_MINOR != 18 || MINT_KVERSION != 2 || !so_register)
	{
		c_conws (MSG_OLDMINT);
		return NULL;
	}
	
	if (buf_init ())
	{
		c_conws ("Cannot initialize buf allocator\n\r");
		c_conws (MSG_FAILURE);
		
		return NULL;
	}
	
	for (r = 0; init_func[r]; r++)
		(*init_func[r])();
	
	c_conws ("\r\n");
	return (DEVDRV *) 1;
}
