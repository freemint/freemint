/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _signal_h
# define _signal_h

# include "mint/mint.h"


long	_cdecl	killgroup	(int pgrp, ushort sig, int priv);
void		post_sig	(PROC *p, ushort sig);
long	_cdecl	ikill		(int p, ushort sig);

void		check_sigs	(void);

void		raise		(ushort sig);
void		bombs		(ushort sig);
void		handle_sig	(ushort sig);
long	_cdecl	p_sigreturn	(void);

void		stop		(ushort sig);
void		exception	(ushort sig);

void		sigbus		(void);
void		sigaddr		(void);
void		sigill		(void);
void		sigpriv		(void);
void		sigfpe		(void);
void		sigtrap		(void);
void		haltformat	(void);
void		haltcpv		(void);


# endif /* _signal_h */
