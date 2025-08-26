/*
 * This file is part of 'minixfs' Copyright 1991,1992,1993 S.N.Henson
 * 
 * Modified for FreeMiNT CVS
 * by Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
 */

# include "bitmap.h"


static long	alloc_bit	(ushort *buf, long num, long last);
static long	free_bit	(ushort *buf, long bitnum);

INLINE long	bitcount	(register ushort wrd);


/* This routine is used for allocating both free inodes and free zones 
 * Search a bitmap for a zero , then return its bit number and change it
 * to a one ...... but without exceeding 'num' bits 
 */

static long
alloc_bit (ushort *buf, register long num, register long last)
{
	register const long end = num >> 4;
	register long i;
	register long j;
	register long k;
	
	k = 1;
	for (i = last >> 4; (i <= end) && (buf[i] == 65535U); i++);
	
	if (i > end)
	{
		return 0;
	}
	else
	{
		for (j = 0; j < 16; j++)
		{
			if (!(buf[i] & k))
			{
				long free;
				
				free = i * 16 + j;
				if (free >= num)
				{
					return 0;
				}
				
				buf[i] |= k;
				
				return free;
			}
			
			k <<= 1;
		}
	}
	
	ALERT (("Minix-FS: alloc_bit: This can't happen !"));
	return 0;
}

/* zero a bit of a bitmap return 0 if already zero */

static long
free_bit (ushort *buf, register long bitnum)
{
	register long index = bitnum >> 4;
	register ushort bit = 1 << (bitnum & 15);
	register long ret;

	ret = buf[index] & bit;
	buf[index] &= ~bit;
	
	return ret;
}


INLINE long
bitcount (register ushort wrd)
{
	register long count = 0;
	register ushort i;
	
	for (i = 1; i != 0; i <<= 1)
	{
		if (wrd & i)
		{
			count++;
		}
	}
	
	return count;
}

long
count_bits (ushort *buf, long num)
{
	register const long end = num >> 4; /* num/16 */
	register long count = 0;
	register long i;
	
	for (i = 0; i < end; i++)
	{
		if (buf[i] == 65535U) count += 16;
		else count += bitcount (buf[i]);
	}
	count += bitcount ((ushort)(buf[end] & ((1L << (num % 16)) - 1L)));
	return count;
}


long
alloc_zone (ushort drive)
{
	SI *psblk = super_ptr[drive];
	long save;
	
	save = alloc_bit (psblk->zbitmap, psblk->sblk->s_zones - psblk->sblk->s_firstdatazn + 1, psblk->zlast);
	if (!save)
	{
		return 0;
	}
	
	psblk->zdirty = 1; /* Mark zone bitmap as dirty */
	if (save > psblk->zlast)
	{
		psblk->zlast = save;
	}
	
	return (save + psblk->sblk->s_firstdatazn - 1);
}

/* Release a zone */
long
free_zone (ushort drive, long zone)
{
	SI *psblk = super_ptr[drive];
	long save;
	long ret;
	
	save = zone + 1 - psblk->sblk->s_firstdatazn;
	ret = free_bit (psblk->zbitmap, save);
	
	/* Mark zone bitmap as dirty */
	psblk->zdirty = 1;
	if (save < psblk->zlast)
	{
		psblk->zlast = save;
	}
	
	if (!ret)
	{
		ALERT (("Minix-FS (%c): zone %ld freeing already free zone !", DriveToLetter(drive), zone));
	}
	
	return ret;
}


ushort
alloc_inode (ushort drive)
{
	SI *psblk = super_ptr[drive];
	ushort save;	
	
	save = alloc_bit (psblk->ibitmap, psblk->sblk->s_ninodes + 1L, psblk->ilast);
	if (!save)
	{
		return 0;
	}
	
	/* Mark inode bitmap as dirty */
	psblk->idirty = 1;
	if (save > psblk->ilast)
	{
		psblk->ilast = save;
	}
	
	return save;
}

/* Release an inode */
long
free_inode (ushort drive, ushort inum)
{
	SI *psblk = super_ptr[drive];
	long ret;
	
	ret = free_bit (psblk->ibitmap, inum);
	if (inum < psblk->ilast)
	{
		psblk->ilast = inum;
	}
	
	/* Mark inode bitmap as dirty */
	psblk->idirty = 1;
	
	if (!ret)
	{
		ALERT (("Minix-FS (%c): inode %d, freeing already free inode!", DriveToLetter(drive), inum));
	}
	
	return ret;
}

