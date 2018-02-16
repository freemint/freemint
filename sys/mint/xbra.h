/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

/*
 * Copyright 1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 */

# ifndef _mint_xbra_h
# define _mint_xbra_h


typedef struct xbra xbra_vec;

struct xbra
{
	long		xbra_magic;
	long		xbra_id;
	xbra_vec	*next;
	short		jump;
	long _cdecl	(*this)();
};


# define XBRA_MAGIC	0x58425241L /* "XBRA" */
# define MINT_MAGIC	0x4d694e54L /* "MiNT" */
# define JMP_OPCODE	0x4EF9


# endif /* _mint_xbra_h */
