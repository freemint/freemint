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
 * begin:	1999-09-27
 * last change: 2000-02-02
 * 
 * Author: Thomas Binder <gryf@hrzpub.tu-darmstadt.de>
 * 
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 */

# ifndef _crypt_IO_h
# define _crypt_IO_h

# include "mint/mint.h"


/*
 * exported functions
 */

void	init_crypt_IO		(void);
long	_cdecl d_setkey		(llong dev, char *key, int cipher);


# endif /* _crypt_IO_h */
