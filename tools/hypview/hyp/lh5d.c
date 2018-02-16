/*
 * HypView - (c)      - 2006 Philipp Donze
 *               2006 -      Philipp Donze & Odd Skancke
 *
 * A replacement hypertext viewer
 *
 * This file is part of HypView.
 *
 * HypView is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * HypView is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HypView; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*********************************************************************
*
* HYPLH5D
*
* Kurzbeschreibung:
* Decoder fuer LH5-Speicherbloecke
*
* Versionen:
* 1.0    mo  01.09.94  Basisversion uebernommen aus LZHSRC-Paket
*
* Autoren:
* mo   (\/) Martin Osieka, Erbacherstr. 2, D-64283 Darmstadt
*
*********************************************************************/

#include <limits.h>

#define DICBIT    13    /* 12(-lh4-) or 13(-lh5-) */
#define DICSIZ (1U << DICBIT)
#define MAXMATCH 256    /* formerly F (not more than UCHAR_MAX + 1) */
#define THRESHOLD  3    /* choose optimal value */
#define NC (UCHAR_MAX + MAXMATCH + 2 - THRESHOLD)	/* 0x1FE */
	/* alphabet = {0, 1, 2, ..., NC - 1} */
#define CBIT 9  /* $\lfloor \log_2 NC \rfloor + 1$ */
#define CODE_BIT  16  /* codeword length */

#define NP (DICBIT + 1)		/* 0xE */
#define NT (CODE_BIT + 3)	/* 0x13 */
#define PBIT 4  /* smallest integer such that (1U << PBIT) > NP */
#define TBIT 5  /* smallest integer such that (1U << TBIT) > NT */
#if NT > NP
	#define NPT NT			/* 19 */
#else
	#define NPT NP
#endif

static unsigned short left[2 * NC - 1];		/* 0x7F6 */
static unsigned short right[2 * NC - 1];	/* 7F6 */
static unsigned char c_len[NC];				/* 1FE */
static unsigned char pt_len[NPT];			/* 13 */
static unsigned short c_table[4096];		/* 2000 */
static unsigned short pt_table[256];		/* 200 */
	          
static unsigned short bitbuf;
#define BITBUFSIZ	(CHAR_BIT * (short)sizeof( bitbuf))

static unsigned short subbitbuf;
static short bitcount;
static short blocksize;


/* Parameterblock */
char *lh5_packedMem;
long lh5_packedLen;
char *lh5_unpackedMem;
long lh5_unpackedLen;
  
/* Local prototype */
void lh5_decode( short reset);


static void
fillbuf( short n)  /* Shift bitbuf n bits left, read n bits */
{
	bitbuf <<= n;
	while (n > bitcount) {
		bitbuf |= subbitbuf << (n -= bitcount);
		if (lh5_packedLen != 0) {
			lh5_packedLen--;
			subbitbuf = (unsigned char) *lh5_packedMem++;
		} else
			subbitbuf = 0;
		bitcount = CHAR_BIT;
	}
	bitbuf |= subbitbuf >> (bitcount -= n);
} /* fillbuf */



static unsigned short
getbits( short n)
{
	unsigned short x;

	if (n == 0)	/* FR*/
		x = 0;	/* FR*/
	else			/* FR*/
		x = bitbuf >> (BITBUFSIZ - n);
	fillbuf(n);
	return x;
} /* getbits */



static void
make_table( short nchar, unsigned char bitlen[], short tablebits, unsigned short table[])
{
	unsigned short count[17], weight[17], start[18], *p;
	unsigned short i, k, len, ch, jutbits, avail, nextcode, mask;

	for (i = 1; i <= 16; i++) count[i] = 0;
	for (i = 0; i < nchar; i++) count[bitlen[i]]++;

	start[1] = 0;
	for (i = 1; i <= 16; i++)
		start[i + 1] = start[i] + (count[i] << (16 - i));
#if 0
	if (start[17] != (unsigned short)(1U << 16)) error("Bad table");
#endif

	jutbits = 16 - tablebits;
	for (i = 1; i <= tablebits; i++) {
		start[i] >>= jutbits;
		weight[i] = 1U << (tablebits - i);
	}
	while (i <= 16) weight[i++] = 1U << (16 - i);

	i = start[tablebits + 1] >> jutbits;
	if (i != (unsigned short)(1U << 16)) {
		k = 1U << tablebits;
		while (i != k) table[i++] = 0;
	}

	avail = nchar;
	mask = 1U << (15 - tablebits);
	for (ch = 0; ch < nchar; ch++) {
		if ((len = bitlen[ch]) == 0) continue;
		nextcode = start[len] + weight[len];
		if (len <= tablebits) {
			for (i = start[len]; i < nextcode; i++) table[i] = ch;
		} else {
			k = start[len];
			p = &table[k >> jutbits];
			i = len - tablebits;
			while (i != 0) {
				if (*p == 0) {
					right[avail] = left[avail] = 0;
					*p = avail++;
				}
				if (k & mask) p = &right[*p];
				else          p = &left[*p];
				k <<= 1;  i--;
			}
			*p = ch;
		}
		start[len] = nextcode;
	}
} /* make_table */



static
void read_pt_len( short nn, short nbit, short i_special)
{
	short i, c, n;
	unsigned short mask;

	n = getbits(nbit);
	if (n == 0) {
		c = getbits(nbit);
		for (i = 0; i < nn; i++) pt_len[i] = 0;
		for (i = 0; i < 256; i++) pt_table[i] = c;
	} else {
		i = 0;
		while (i < n) {
			c = bitbuf >> (BITBUFSIZ - 3);
			if (c == 7) {
				mask = 1U << (BITBUFSIZ - 1 - 3);
				while (mask & bitbuf) {  mask >>= 1;  c++;  }
			}
			fillbuf((c < 7) ? 3 : c - 3);
			pt_len[i++] = c;
			if (i == i_special) {
				c = getbits(2);
				while (--c >= 0) pt_len[i++] = 0;
			}
		}
		while (i < nn) pt_len[i++] = 0;
		make_table(nn, pt_len, 8, pt_table);
	}
} /* read_pt_len */



static void
read_c_len( void)
{
	short i, c, n;
	unsigned short mask;

	n = getbits(CBIT);
	if (n == 0) {
		c = getbits(CBIT);
		for (i = 0; i < NC; i++) c_len[i] = 0;
		for (i = 0; i < 4096; i++) c_table[i] = c;
	} else {
		i = 0;
		while (i < n) {
			c = pt_table[bitbuf >> (BITBUFSIZ - 8)];
			if (c >= NT) {
				mask = 1U << (BITBUFSIZ - 1 - 8);
				do {
					if (bitbuf & mask) c = right[c];
					else               c = left [c];
					mask >>= 1;
				} while (c >= NT);
			}
			fillbuf(pt_len[c]);
			if (c <= 2) {
				if      (c == 0) c = 1;
				else if (c == 1) c = getbits(4) + 3;
				else             c = getbits(CBIT) + 20;
				while (--c >= 0) c_len[i++] = 0;
			} else c_len[i++] = c - 2;
		}
		while (i < NC) c_len[i++] = 0;
		make_table(NC, c_len, 12, c_table);
	}
} /* read_c_len */



static unsigned short
decode_c( void)
{
	unsigned short j, mask;

	if (blocksize == 0) {
		blocksize = getbits(16);
		read_pt_len(NT, TBIT, 3);
		read_c_len();
		read_pt_len(NP, PBIT, -1);
	}
	blocksize--;
	j = c_table[bitbuf >> (BITBUFSIZ - 12)];
	if (j >= NC) {
		mask = 1U << (BITBUFSIZ - 1 - 12);
		do {
			if (bitbuf & mask) j = right[j];
			else               j = left [j];
			mask >>= 1;
		} while (j >= NC);
	}
	fillbuf(c_len[j]);
	return j;
} /* decode_c */



static unsigned short
decode_p( void)
{
	unsigned short j, mask;

	j = pt_table[bitbuf >> (BITBUFSIZ - 8)];
	if (j >= NP) {
		mask = 1U << (BITBUFSIZ - 1 - 8);
		do {
			if (bitbuf & mask) j = right[j];
			else               j = left [j];
			mask >>= 1;
		} while (j >= NP);
	}
	fillbuf(pt_len[j]);
	if (j != 0) j = (1U << (j - 1)) + getbits(j - 1);
	return j;
} /* decode_p */

void
lh5_decode( short reset)
/* In der Assemblerversion ist decode_c und decode_p inline kodiert. */
{
	static short dec_j;  /* remaining bytes to copy */
	unsigned short c;
#if 0
	static unsigned short i;
	unsigned short r;
#else	/* MO 8/95: Umstellung auf LONG, da entpackte Seiten > 64K sein koennen */
	static long i;
	long r;
#endif

	r = 0;

	if (reset) {
		bitbuf = 0;
		subbitbuf = 0;
		bitcount = 0;
		fillbuf( BITBUFSIZ);
		blocksize = 0;
		dec_j = 0;
	}
	else {
		while (--dec_j >= 0) {
			lh5_unpackedMem[r] = lh5_unpackedMem[i];
#if 0
			i = (i + 1) & (DICSIZ - 1);
#else
			i++;
#endif
			if (++r == lh5_unpackedLen)
				return;
		}
	};

	for ( ; ; ) {

		c = decode_c();

		if (c <= UCHAR_MAX) {		/* Zeichen direkt uebernehmen */
			lh5_unpackedMem[r] = c;
			if (++r == lh5_unpackedLen)
				return;
		}
		else {						/* Zeichen ist Laenge des einzufuegenden Textsegments */
			dec_j = c - (UCHAR_MAX + 1 - THRESHOLD);
#if 0
			i = (r - decode_p() - 1) & (DICSIZ - 1);
#else
			i = (r - decode_p() - 1);
#endif
			while (--dec_j >= 0) {
				lh5_unpackedMem[r] = lh5_unpackedMem[i];
#if 0
				i = (i + 1) & (DICSIZ - 1);
#else
				i++;
#endif
				if (++r == lh5_unpackedLen)
					return;
			}
		}
	}
} /* lh5_decode */

