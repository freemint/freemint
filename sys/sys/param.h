/*
 * $Id$
 * 
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution.
 * 
 */

#ifndef _SYS_PARAM_H
#define _SYS_PARAM_H 1

#define	MAXPATHLEN	128		/* same as FILENAME_MAX in stdio.h */
#define	NOFILE		20		/* same as OPEN_MAX in limits.h */

/* Only define MAXHOSTNAMELEN if portlib.h got not included!  I wish
   the portlib was as kind as we are.  */
#ifndef MAXHOSTNAMELEN
# define MAXHOSTNAMELEN 64
#endif

#ifdef __MINT__
# define HZ		200		/* ticks/second reported by times() */
# define NCARGS		1024		/* actually, we don't limit this */
#else
# define HZ		60		/* ticks/second reported by times() */
# define NCARGS		126		/* max. no. of characters in argv */
#endif

#define FSCALE 2048

/* Bit map related macros.  */
#define	setbit(a,i)	((a)[(i)/NBBY] |= 1<<((i)%NBBY))
#define	clrbit(a,i)	((a)[(i)/NBBY] &= ~(1<<((i)%NBBY)))
#define	isset(a,i)	((a)[(i)/NBBY] & (1<<((i)%NBBY)))
#define	isclr(a,i)	(((a)[(i)/NBBY] & (1<<((i)%NBBY))) == 0)

/* Macros for counting and rounding.  */
#ifndef howmany
# define howmany(x, y)	(((x)+((y)-1))/(y))
#endif
#define	roundup(x, y)	((((x)+((y)-1))/(y))*(y))
#define powerof2(x)	((((x)-1)&(x))==0)

/* Unit of `st_blocks'.  */
#define DEV_BSIZE       512


#endif /* _SYS_PARAM_H */
