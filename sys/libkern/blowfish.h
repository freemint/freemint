/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 * 
 * 
 * blowfish.h dated 99/09/26
 * 
 * Constants and prototypes for the Blowfish cipher algorithm. Original file
 * from Bruce Schneier, with modifications to handle multiple keys at a time.
 * 
 */

# ifndef _blowfish_h
# define _blowfish_h

# include "mint/mint.h"

# define bf_N 16

/* This is the internal representation of a Blowfish key (Gryf) */
struct bf_key
{
	unsigned long bf_P[bf_N + 2];
	unsigned long bf_S[4][256];
};

void  Blowfish_encipher  (struct bf_key *bfk, unsigned long *xl, unsigned long *xr);
void  Blowfish_decipher  (struct bf_key *bfk, unsigned long *xl, unsigned long *xr);
short Blowfish_initialize(struct bf_key *bfk, unsigned char key[], short keybytes);

# endif /* _blowfish_h */
