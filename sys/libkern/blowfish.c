/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 * 
 * 
 * blowfish.c dated 99/09/26
 * 
 * Implements the Blowfish cipher algorithm. It's in fact the original code
 * from Bruce Schneier, enhanced so that it can handle multiple keys at a time
 * (as the code found in "Applied Cryptography", I don't know why he dropped
 * this in the later version).
 * 
 */

# include "blowfish.h"

# include "libkern/libkern.h"


/* TODO: test with zero length key */
/* TODO: test with a through z as key and plain text */
/* TODO: make this byte order independent */

# define MAXKEYBYTES	56		/* 448 bits */
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
	unsigned long word;
	unsigned char byte [4];
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
	unsigned long word;
	unsigned char byte [4];
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
	unsigned long word;
	unsigned char byte [4];
	struct
	{
		unsigned int byte1:8;
		unsigned int byte0:8;
		unsigned int byte3:8;
		unsigned int byte2:8;
	} w;
};
# endif /* ORDER_BADC */

/* P-box P-array, S-box  */
# include "bf_tab.h"

# define S(k,x,i) (k->bf_S[i][x.w.byte##i])
# define bf_F(k,x) (((S(k,x,0) + S(k,x,1)) ^ S(k,x,2)) + S(k,x,3))
# define BLOWFISH_ROUND(k,a,b,n) (a.word ^= bf_F(k,b) ^ k->bf_P[n])

void
Blowfish_encipher(struct bf_key *bfk, unsigned long *xl, unsigned long *xr)
{
	union aword Xl;
	union aword Xr;
	
	Xl.word = *xl;
	Xr.word = *xr;
	
	Xl.word ^= bfk->bf_P[0];
	BLOWFISH_ROUND(bfk, Xr, Xl,  1);
	BLOWFISH_ROUND(bfk, Xl, Xr,  2);
	BLOWFISH_ROUND(bfk, Xr, Xl,  3);
	BLOWFISH_ROUND(bfk, Xl, Xr,  4);
	BLOWFISH_ROUND(bfk, Xr, Xl,  5);
	BLOWFISH_ROUND(bfk, Xl, Xr,  6);
	BLOWFISH_ROUND(bfk, Xr, Xl,  7);
	BLOWFISH_ROUND(bfk, Xl, Xr,  8);
	BLOWFISH_ROUND(bfk, Xr, Xl,  9);
	BLOWFISH_ROUND(bfk, Xl, Xr, 10);
	BLOWFISH_ROUND(bfk, Xr, Xl, 11);
	BLOWFISH_ROUND(bfk, Xl, Xr, 12);
	BLOWFISH_ROUND(bfk, Xr, Xl, 13);
	BLOWFISH_ROUND(bfk, Xl, Xr, 14);
	BLOWFISH_ROUND(bfk, Xr, Xl, 15);
	BLOWFISH_ROUND(bfk, Xl, Xr, 16);
	Xr.word ^= bfk->bf_P[17];
	
	*xr = Xl.word;
	*xl = Xr.word;
}

void
Blowfish_decipher(struct bf_key *bfk, unsigned long *xl, unsigned long *xr)
{
	union aword Xl;
	union aword Xr;
	
	Xl.word = *xl;
	Xr.word = *xr;
	
	Xl.word ^= bfk->bf_P[17];
	BLOWFISH_ROUND(bfk, Xr, Xl, 16);
	BLOWFISH_ROUND(bfk, Xl, Xr, 15);
	BLOWFISH_ROUND(bfk, Xr, Xl, 14);
	BLOWFISH_ROUND(bfk, Xl, Xr, 13);
	BLOWFISH_ROUND(bfk, Xr, Xl, 12);
	BLOWFISH_ROUND(bfk, Xl, Xr, 11);
	BLOWFISH_ROUND(bfk, Xr, Xl, 10);
	BLOWFISH_ROUND(bfk, Xl, Xr,  9);
	BLOWFISH_ROUND(bfk, Xr, Xl,  8);
	BLOWFISH_ROUND(bfk, Xl, Xr,  7);
	BLOWFISH_ROUND(bfk, Xr, Xl,  6);
	BLOWFISH_ROUND(bfk, Xl, Xr,  5);
	BLOWFISH_ROUND(bfk, Xr, Xl,  4);
	BLOWFISH_ROUND(bfk, Xl, Xr,  3);
	BLOWFISH_ROUND(bfk, Xr, Xl,  2);
	BLOWFISH_ROUND(bfk, Xl, Xr,  1);
	Xr.word ^= bfk->bf_P[0];
	
	*xl = Xr.word;
	*xr = Xl.word;
}

short
Blowfish_initialize(struct bf_key *bfk, unsigned char key[], short keybytes)
{
	short i;		/* FIXME: unsigned int, char? */
	short j;		/* FIXME: unsigned int, char? */
	unsigned long data;
	unsigned long datal;
	unsigned long datar;
	union aword temp;
	
	memcpy(bfk->bf_P, bf_P, sizeof(bf_P));
	memcpy(bfk->bf_S, bf_S, sizeof(bf_S));
	
	j = 0;
	for (i = 0; i < bf_N + 2; ++i)
	{
		temp.word = 0;
		temp.w.byte0 = key[j];
		temp.w.byte1 = key[(j+1)%keybytes];
		temp.w.byte2 = key[(j+2)%keybytes];
		temp.w.byte3 = key[(j+3)%keybytes];
		data = temp.word;
		bfk->bf_P[i] = bfk->bf_P[i] ^ data;
		j = (j + 4) % keybytes;
	}
	
	datal = 0x00000000;
	datar = 0x00000000;
	
	for (i = 0; i < bf_N + 2; i += 2)
	{
		Blowfish_encipher(bfk, &datal, &datar);

		bfk->bf_P[i] = datal;
		bfk->bf_P[i + 1] = datar;
	}

	for (i = 0; i < 4; ++i)
	{
		for (j = 0; j < 256; j += 2)
		{
			Blowfish_encipher(bfk, &datal, &datar);
	 
			bfk->bf_S[i][j] = datal;
			bfk->bf_S[i][j + 1] = datar;
		}
	}
	
	return 0;
}
