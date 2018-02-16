/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

/*
 * Copyright 1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 */

# ifndef _mint_kassert_h
# define _mint_kassert_h

# ifndef __KERNEL__
# error __KERNEL__ not defined!
# endif

# ifdef assert
# undef assert
# endif

#if 0
# define assert(x)
#else
# define assert(expression)						\
	((expression) ? (void)0 :					\
		FATAL ("assert (`%s') failed at line %ld of %s.",	\
			#expression, (long) __LINE__, __FILE__))

#endif

# endif /* _mint_kassert_h */
