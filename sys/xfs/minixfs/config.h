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

# ifndef _config_h
# define _config_h


/* User settable parameters */

/*# define MFS_NMOVE_DIR*/	/* no movement of directories (rename OK) */

# define PRE_READ	8	/* Max Number of blocks to 'read-ahead' */
# define MAX_RWS	1024	/* Maximum sectors to read/write atomically */

/* Default translation modes ... change if desired */
# define TRANS_DEFAULT	(SRCH_TOS | DIR_TOS | DIR_MNT | LWR_TOS | AEXEC_TOS)

# if 1
/* Set this to be true if 'c' is an illegal character for a filename */
# define BADCHR(c)	(c == '/')
# else
/* Some unixes have something a bit stricter, uncomment out this if required */
# define BADCHR(c)	((c == '/') || (c & 0x80))
# endif


# endif /* _config_h */
