/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

/*
 * Copyright 1990,1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 */

# ifndef _mint_mint_h
# define _mint_mint_h

# include "kernel.h"

# include "kassert.h"
# include "kcompiler.h"
# include "ktypes.h"
# include "errno.h"


# if __KERNEL__ == 1

# include "config.h"
# include "debug.h"

# define yield()	sleep(READY_Q, 0L)

# endif


# endif /* _mint_mint_h */
