/*
 * $Id$
 * 
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _init_h
# define _init_h

# include "mint/mint.h"
# include "mint/basepage.h"
# include "mint/emu_tos.h"

extern KBDVEC *syskey;
extern short intr_done;
extern short step_by_step;

void	boot_print	(const char *s);
void	boot_printf	(const char *fmt, ...);
void	stop_and_ask	(void);

void	new_xbra_install(long *xv, long addr, long _cdecl (*func)());

void	restr_intr	(void);
void	init		(void);
void	install_cookies	(void);

long	env_size	(const char *var);
void	_mint_delenv	(BASEPAGE *bp, const char *strng);
void	_mint_setenv	(BASEPAGE *bp, const char *var, const char *value);

# endif /* _init_h */
