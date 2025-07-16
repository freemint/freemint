/*
 * This file is part of 'minixfs' Copyright 1991,1992,1993 S.N.Henson
 * 
 * Modified for FreeMiNT CVS
 * by Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
 */

# ifndef _global_h
# define _global_h

# include "mint/mint.h"

# include "libkern/libkern.h"
# include "mint/file.h"
# include "mint/time.h"

/* own default header */
# include "config.h"
# include "minix.h"


/* debug section
 */

# if 1
# define FS_DEBUG	1
# endif

# ifdef FS_DEBUG

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


/* UTC support
 */

INLINE time64_t current_time (void)
{
	if (KERNEL->xtime64)
		return KERNEL->xtime64->tv_sec;
	return (u_int32_t)KERNEL->xtime->tv_sec;
}
# define CURRENT_TIME	current_time ()


/* error UNIT */
extern UNIT error;

/* Pointer to drive info */
extern SI *super_ptr [NUM_DRIVES];

/* First FILEPTR in chained list */
extern FILEPTR *firstptr;

/*
 * Binary readable parameters
 */

/* Magic number */
extern long mfs_magic;

/* Minixfs version */
extern int mfs_maj;
extern int mfs_min;
extern int mfs_plev;

/*
 * Binary configurable parameters
 */

/* Translation modes */
extern long fs_mode[NUM_DRIVES];


# endif /* _global_h */
