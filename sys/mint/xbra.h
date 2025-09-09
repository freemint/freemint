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

/* XBRA is a convention used in TOS programming when hooking vectors:
 * When a vector is hooked:
 * - the long at address "handler-12" is 'XBRA', to indicate compliance with this convention.
 * - the long at address "handler-8" is a 4 ASCII character id of the owner of the handler
 * - the long at address "handler-4" is the previous handler
 * - the long at address "handler" is the handler itself (obviously).
 * The below helps implement that.
 */

typedef struct xbra xbra_vec;

struct xbra
{
	long		xbra_magic; /* should be XBRA_MAGIC     */
	long		xbra_id;    /* our id is MINT_MAGIC     */
	xbra_vec	*next;      /* original handler         */
	short		jump;       /* this does "jump to this" */
	long _cdecl	(*this)();
};


# define XBRA_MAGIC	0x58425241L /* "XBRA" */
# define MINT_MAGIC	0x4d694e54L /* "MiNT" */
# define JMP_OPCODE	0x4EF9


# endif /* _mint_xbra_h */
