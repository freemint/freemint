/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _signal_h
# define _signal_h

# include "mint/mint.h"


long	_cdecl	killgroup	(int pgrp, ushort sig, int priv);
void		post_sig	(struct proc *p, ushort sig);
long	_cdecl	ikill		(int p, ushort sig);
void		check_sigs	(void);
void	_cdecl	raise		(ushort sig);
void		handle_sig	(ushort sig);
void	_cdecl	stop		(ushort sig);

# endif /* _signal_h */
