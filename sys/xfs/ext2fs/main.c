/*
 * Filename:     main.c
 * Project:      ext2 file system driver for MiNT
 * 
 * Note:         Please send suggestions, patches or bug reports to me
 *               or the MiNT mailing list (mint@).
 * 
 * Copying:      Copyright 1999 Frank Naumann (fnaumann@freemint.de)
 *               Copyright 1998, 1999 Axel Kaiser (DKaiser@AM-Gruppe.de)
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

# include "global.h"

# include "ext2sys.h"
# include "version.h"
# include "buildinfo/version.h"


# define MSG_VERSION	str (VER_MAJOR) "." str (VER_MINOR)
# define MSG_BUILDDATE	__DATE__

# define MSG_BOOT	\
	"\033p Ext2 filesystem driver version " MSG_VERSION " \033q\r\n"

# define MSG_GREET	\
	"\xbd 1998, 1999 by Axel Kaiser.\r\n" \
	"\xbd 2000-2010 by Frank Naumann.\r\n"

# define MSG_ALPHA	\
	"\033p WARNING: This is an unstable version - ALPHA! \033q\7\r\n"

# define MSG_BETA	\
	"\033p WARNING: This is a test version - BETA! \033q\7\r\n"

# define MSG_INCVERS	\
	"\033pIncorrect MiNT kernel version!\033q\r\n"

# define MSG_BIOVERSION	\
	"\033pIncompatible FreeMiNT buffer cache version!\033q\r\n"

# define MSG_BIOREVISION	\
	"\033pFreeMiNT buffer cache revision too old!\033q\r\n"

# define MSG_FAILURE	\
	"\7Sorry, module NOT installed!\r\n\r\n"

static int maj_version = MINT_MAJ_VERSION;
static int min_version = MINT_MIN_VERSION;

FILESYS * init(struct kerinfo *k);

struct kerinfo *KERNEL;
FILESYS *
init (struct kerinfo *k)
{
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
	
	KERNEL_DEBUG ("ext2 (%s): init", __FILE__);
	
	/* version check */
	if (MINT_MAJOR != maj_version && MINT_MINOR != min_version)
	{
		c_conws (MSG_INCVERS);
		c_conws (MSG_FAILURE);
		
		return NULL;
	}
	
	/* check buffer cache version */
	if (bio.version != 3)
	{
		c_conws (MSG_BIOVERSION);
		c_conws (MSG_FAILURE);
		
		return NULL;		
	}
	
	/* check for revision 1 features */
	if (bio.revision < 1)	
	{
		c_conws (MSG_BIOREVISION);
		c_conws (MSG_FAILURE);
		
		return NULL;		
	}
	
# if 1
	/* check for native UTC timestamps */
	if (MINT_KVERSION > 0 && KERNEL->xtime)
	{
		/* yeah, save enourmous overhead */
		native_utc = 1;
		
		KERNEL_DEBUG ("ext2 (%s): running in native UTC mode!", __FILE__);
	}
	else
# endif
	{
		/* disable extension level 3 */
		ext2_filesys.fsflags &= ~FS_EXT_3;
	}
	
	KERNEL_DEBUG ("ext2 (%s): loaded and ready (k = %lx) -> %lx.", __FILE__, k, (long) &ext2_filesys);
	return &ext2_filesys;
}
