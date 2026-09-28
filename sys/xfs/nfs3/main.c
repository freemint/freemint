/*
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 *
 * NFS version 3 (RFC 1813) support, derived from the NFS version 2
 * driver. See the file COPYING for copying and using conditions.
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
	"\033p NFS version 3 file system driver version " MSG_VERSION " \033q\r\n"

# define MSG_GREET	\
	"Derived from the NFS v2 driver, see RFC 1813.\r\n" \
	"See the file COPYING for copying and using conditions.\r\n"

# define MSG_ALPHA	\
	"\033p WARNING: This is an unstable version - ALPHA! \033q\7\r\n"

# define MSG_BETA	\
	"\033p WARNING: This is a test version - BETA! \033q\7\r\n"

# define MSG_OLDMINT	\
	"\033pMiNT too old, this xfs requires at least a FreeMiNT 1.16!\033q\r\n"

# define MSG_FAILURE(s)	\
	"\7Sorry, nfs3.xfs NOT installed: " s "!\r\n\r\n"


struct kerinfo *KERNEL;

FILESYS *_cdecl init_xfs (struct kerinfo *k);

FILESYS *_cdecl
init_xfs (struct kerinfo *k)
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

		DEBUG(("nfs3 (%s): running in native UTC mode!", __FILE__));
	}
	else
	{
		/* disable extension level 3 */
		DEBUG(("nfs3 (%s): old kernel, disabling UTC mode!", __FILE__));
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

	r = d_cntl (FS_MOUNT, NFS3_MOUNTPOINT, (long) &d);
	DEBUG(("d_cntl(FS_MOUNT): r=%ld nfs_dev=%d", r, d.dev_no));
	if (r == d.dev_no)
	{
		nfs_dev = d.dev_no;
		return (FILESYS *) 1L;
	}

	c_conws (MSG_FAILURE ("Dcntl(FS_MOUNT) failed"));

	if (d_cntl (FS_UNINSTALL, NFS3_MOUNTPOINT, (long) &d))
	{
		/* can't return NULL here because FS_UNINSTALL failed */
		return (FILESYS *) 1;
	}

	return NULL;
}
