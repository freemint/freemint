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

# ifndef _misc_h
# define _misc_h

# include "global.h"


int	inode_busy	(unsigned inum, int drive, int flag);
void	btos_cpy	(char *to, const char *from);
int	stob_ncpy	(char *to, const char *from, long n);


INLINE int check_mode	(int euid, int egid, d_inode *rip, int access);


/* Check for access 'access' for given uid/gid/mode
 * return 0 if access allowed.
 */

INLINE int
check_mode (int euid, int egid, d_inode *rip, int access)
{
	if (!euid) return 0;
	if (euid == rip->i_uid && (rip->i_mode & access)) return 0;
	if (egid == rip->i_gid && (rip->i_mode & (access >> 3))) return 0;
	if (rip->i_mode & (access >>6)) return 0;
	return 1;
}


# endif /* _misc_h */
