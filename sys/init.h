/*
 * $Id$
 * 
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _init_h
# define _init_h

# include "mint/mint.h"

void	boot_print	(const char *s);
void	boot_printf	(const char *fmt, ...);

void	restr_intr	(void);
void	init		(void);
void	install_cookies	(void);

extern char *sysdir;

# endif /* _init_h */
