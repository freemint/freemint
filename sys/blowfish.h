/*
 * $Id$
 * 
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


# ifdef CRYPTO_CODE

/* $Id$*/

# define MAXKEYBYTES	56		/* 448 bits */
# define bf_N		16
# define noErr		0
# define DATAERROR	-1
# define KEYBYTES	8


/* choose a byte order for your hardware */
/* Does ANSI actually define the memory layout of bitfields? (Gryf) */
# define ORDER_ABCD

/* ABCD - big endian - motorola */
# ifdef ORDER_ABCD
union aword
{
	ulong word;
	uchar byte [4];
	struct
	{
		unsigned int byte0:8;
		unsigned int byte1:8;
		unsigned int byte2:8;
		unsigned int byte3:8;
	} w;
};
# endif /* ORDER_ABCD */

/* DCBA - little endian - intel */
# ifdef ORDER_DCBA
union aword
{
	ulong word;
	uchar byte [4];
	struct
	{
		unsigned int byte3:8;
		unsigned int byte2:8;
		unsigned int byte1:8;
		unsigned int byte0:8;
	} w;
};
# endif /* ORDER_DCBA */

/* BADC - vax */
# ifdef ORDER_BADC
union aword
{
	ulong word;
	uchar byte [4];
	struct
	{
		unsigned int byte1:8;
		unsigned int byte0:8;
		unsigned int byte3:8;
		unsigned int byte2:8;
	} w;
};
# endif /* ORDER_BADC */

/* This is the internal representation of a Blowfish key (Gryf) */
struct bf_key
{
	ulong	bf_P [bf_N + 2];
	ulong	bf_S [4][256];
};

void Blowfish_encipher	(BF_KEY *bfk, ulong *xl, ulong *xr);
void Blowfish_decipher	(BF_KEY *bfk, ulong *xl, ulong *xr);
short InitializeBlowfish(BF_KEY *bfk, uchar key[], short keybytes);

# endif /* CRYPTO_CODE */

# endif /* _blowfish_h */
