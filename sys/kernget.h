/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

/*
 * This file is dedicated to the FreeMiNT project.
 * It's not allowed to use this file for other projects without my
 * explicit permission.
 */

/*
 * begin:	1999-10-24
 * last change: 2000-04-08
 * 
 * Author: Guido Flohr <guido@freemint.de>
 * 
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 * 
 * Kernel system info filesystem, information retrieval routines.
 * 
 */  

# ifndef _kernget_h
# define _kernget_h

# include "mint/mint.h"


# if WITH_KERNFS

long kern_get_unimplemented	(SIZEBUF **buffer);

long kern_get_buildinfo		(SIZEBUF **buffer);
long kern_get_cookiejar		(SIZEBUF **buffer);
long kern_get_filesystems	(SIZEBUF **buffer);
long kern_get_hz		(SIZEBUF **buffer);
long kern_get_loadavg		(SIZEBUF **buffer);
long kern_get_time		(SIZEBUF **buffer);
long kern_get_uptime		(SIZEBUF **buffer);
long kern_get_version		(SIZEBUF **buffer);
long kern_get_welcome		(SIZEBUF **buffer);

long kern_procdir_get_cmdline	(SIZEBUF **buffer, const PROC *p);
long kern_procdir_get_environ	(SIZEBUF **buffer,       PROC *p);
long kern_procdir_get_fname	(SIZEBUF **buffer, const PROC *p);
long kern_procdir_get_meminfo	(SIZEBUF **buffer, const PROC *p);
long kern_procdir_get_stat	(SIZEBUF **buffer,       PROC *p);
long kern_procdir_get_status	(SIZEBUF **buffer, const PROC *p);

# endif


# endif /* _kernget_h */
