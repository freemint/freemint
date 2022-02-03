/*
 * Copyright 1993, 1994 by Ulrich K�hn. All rights reserved.
 *
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 */

/*
 * File : main.c 
 *        installation functions
 */

# include "global.h"
# include "nfssys.h"
# include "version.h"

# include "mint/dcntl.h"


# define MSG_VERSION	str (VER_MAJOR) "." str (VER_MINOR)
# define MSG_BUILDDATE	__DATE__

# define MSG_BOOT	\
	"\033p Network file system driver version " MSG_VERSION " \033q\r\n"

# define MSG_GREET	\
	"\xbd 1993, 1994 by Ulrich K\x81hn.\r\n" \
	"\xbd 2000-2010 by Frank Naumann.\r\n" \
	"See the file COPYING for copying and using conditions.\r\n"

# define MSG_ALPHA	\
	"\033p WARNING: This is an unstable version - ALPHA! \033q\7\r\n"

# define MSG_BETA	\
	"\033p WARNING: This is a test version - BETA! \033q\7\r\n"

# define MSG_OLDMINT	\
	"\033pMiNT too old, this xfs requires at least a FreeMiNT 1.16!\033q\r\n"

# define MSG_BIOVERSION	\
	"\033pIncompatible FreeMiNT buffer cache version!\033q\r\n"

# define MSG_BIOREVISION	\
	"\033pFreeMiNT buffer cache revision too old!\033q\r\n"

# define MSG_FAILURE(s)	\
	"\7Sorry, nfs.xfs NOT installed: " s "!\r\n\r\n"


struct kerinfo *KERNEL;

FILESYS * init(struct kerinfo *k);

FILESYS *
init (struct kerinfo *k)
{
	struct fs_descr d = { &nfs_filesys, -1 };
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
	
	
	/* version check */
	if ((MINT_MAJOR < 1)
	    || (MINT_MAJOR == 1 && MINT_MINOR < 16)
	    || (!so_create))
	{
		c_conws (MSG_OLDMINT);
		c_conws (MSG_FAILURE ("MiNT too old"));
		
		return NULL;
	}
	
	/* check for native UTC timestamps */
	if (MINT_KVERSION > 0 && KERNEL->xtime)
	{
		/* yeah, save enourmous overhead */
		native_utc = 1;
		
		KERNEL_DEBUG ("nfs (%s): running in native UTC mode!", __FILE__);
	}
	else
	{
		/* disable extension level 3 */
		nfs_filesys.fsflags &= ~FS_EXT_3;
	}
	
	/* initialize the other services in the xfs */
	init_fs ();
	
	r = d_cntl (FS_INSTALL, "u:\\", (long) &d);
	if (r != (long) kernel)
	{
		c_conws (MSG_FAILURE ("Dcntl(FS_INSTALL) failed"));
		return NULL;
	}
	
	r = d_cntl (FS_MOUNT, "u:\\nfs", (long) &d);
	if (r == d.dev_no)
	{
		nfs_dev = d.dev_no;
		return (FILESYS *) 1L;
	}
	
	c_conws (MSG_FAILURE ("Dcntl(FS_MOUNT) failed"));
	
	if (d_cntl (FS_UNINSTALL, "u:\\nfs", (long) &d))
	{
		/* can't return NULL here because FS_UNINSTALL failed */
		return (FILESYS *) 1;
	}
	
	return NULL;
}
