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

# include "zone.h"

# include "bitmap.h"
# include "inode.h"


void
read_zone (long num, void *buf, int drive)
{
	if (num)
	{
		UNIT *u = bio.read (super_ptr[drive]->di, num, BLOCK_SIZE);
		if (u)
			bcopy (u->data, buf, BLOCK_SIZE);
		else
			bzero (buf, BLOCK_SIZE);
	}
	else
		bzero (buf, BLOCK_SIZE);
}

void
write_zone (long num, void *buf, int drive)
{
	UNIT *u = bio.getunit (super_ptr[drive]->di, num, BLOCK_SIZE);
	if (u)
	{
		bcopy (buf, u->data, BLOCK_SIZE);
		bio_MARK_MODIFIED (&(bio), u);
	}
}

UNIT *
cget_zone (long num, int drive)
{
	UNIT *u;
	
	u = bio.read (super_ptr[drive]->di, num, BLOCK_SIZE);
	if (u)
	{
		return u;
	}
	
	bzero (error.data, BLOCK_SIZE);
	return &error;
}

UNIT *
cput_zone (long num, int drive)
{
	UNIT *u;
	
	u = bio.getunit (super_ptr[drive]->di, num, BLOCK_SIZE);
	if (u)
	{
		bzero (u->data, BLOCK_SIZE);
		bio_MARK_MODIFIED (&(bio), u);
		return u;
	}
	
	bzero (error.data, BLOCK_SIZE);
	return &error;
}

/* This routine finds the zone 'numr' of an inode, traversing indirect
 * and double indirect zones as required if flag != 0 zones are added as
 * required. Having two filesystem versions makes this a bit trickier, in
 * fact although using a single routine looks more elegant it is slow,
 * so two versions are used.
 */
/* Special case for l_write:
   if flag > 1, and a new zone is allocated, prepare a cleared block.
   This is for writing to sparse files, when only a partial block is
   written, the rest must be cleared.  Since directories are never
   sparse, always use usrcache in this case. */

long
find_zone (d_inode *rip, long int numr, int drive, int flag)
{
	long temp_zone;
	long *zptr, *zptr2;
	UNIT *tmp, *tmp2, *tmp3;

	/* Past EOF ? */
	if ((numr * BLOCK_SIZE >= rip->i_size) && !flag)
		return 0;

	/* Zone in inode ? */
	if (numr < NR_DZONE_NUM2)
	{
		temp_zone = rip->i_zone[numr];
		if (temp_zone || !flag) return temp_zone;
		temp_zone = (rip->i_zone[numr] = alloc_zone (drive));
		goto new_zone;
	}
	numr -= NR_DZONE_NUM2;
	
	/* In indirect zone then ? */
	if (numr < NR_INDIRECTS2)
	{
		if (rip->i_zone[7])
		{
			tmp = cget_zone (rip->i_zone[7], drive);
			zptr = &((bufr *) tmp->data)->bind[numr];
			if (*zptr || !flag) return *zptr;
			if ((*zptr = alloc_zone (drive))) bio_MARK_MODIFIED (&(bio), tmp);
			temp_zone = *zptr;
			goto new_zone;
		}
		else
		{
			if (!flag || !(rip->i_zone[7] = alloc_zone (drive))) return 0;
			tmp = cput_zone(rip->i_zone[7], drive);
			temp_zone = ((bufr *) tmp->data)->bind[numr] = alloc_zone (drive);
			goto new_zone;
		}
	}
	
	/* Erk double indirect .... */
	numr -= NR_INDIRECTS2;
	if (numr < NR_INDIRECTS2 * NR_INDIRECTS2)
	{
		if (rip->i_zone[8])
		{
			tmp2 = cget_zone (rip->i_zone[8], drive);
			zptr2 = &((bufr *) tmp2->data)->bind[numr >> LNR_IND2];
			if (*zptr2)
			{
		     		tmp = cget_zone (*zptr2, drive);
				zptr = &((bufr *) tmp->data)->bind[numr & (NR_INDIRECTS2 - 1)];
				if (*zptr || !flag) return *zptr;
				if ((*zptr = alloc_zone (drive))) bio_MARK_MODIFIED (&(bio), tmp);
				temp_zone = *zptr;
				goto new_zone;
		  	}
		  	else 
		  	{
		     		if (!flag || !(*zptr2 = alloc_zone (drive))) return 0;
				bio_MARK_MODIFIED (&(bio), tmp2);
				tmp = cput_zone (*zptr2, drive);
				zptr = &((bufr *) tmp->data)->bind[numr & (NR_INDIRECTS2 - 1)];
				temp_zone = *zptr = alloc_zone (drive);
				goto new_zone;
		  	}
		}
		if (!flag || !(rip->i_zone[8] = alloc_zone (drive))) return 0;
	
		tmp2 = cput_zone (rip->i_zone[8], drive);
		zptr2 = &((bufr *) tmp2->data)->bind[numr >> LNR_IND2];
		if (!(*zptr2 = alloc_zone (drive))) return 0;

		tmp = cput_zone (*zptr2, drive);
		zptr = &((bufr *) tmp->data)->bind[numr & (NR_INDIRECTS2 - 1)];
		temp_zone = *zptr = alloc_zone (drive) ;
		goto new_zone;
	}
	
	/* Triple indirect */
	numr -= NR_INDIRECTS2 * NR_INDIRECTS2;
	if (numr < NR_INDIRECTS2 * NR_INDIRECTS2 * NR_INDIRECTS2)
	{
		long *zptr3;
		if (rip->i_zone[9])
		{
			tmp3 = cget_zone (rip->i_zone[9], drive);
			zptr3 = &((bufr *) tmp3->data)->bind[numr >> (LNR_IND2 * 2)];
			if (*zptr3)
			{
				tmp2 = cget_zone (*zptr3, drive);
				zptr2 = &((bufr *) tmp2->data)->bind[(numr >> LNR_IND2) & (NR_INDIRECTS2 - 1)];
				if (*zptr2)
				{
					tmp = cget_zone (*zptr2, drive);
					zptr = &((bufr *) tmp->data)->bind[numr & (NR_INDIRECTS2-1)];
					if (*zptr || !flag)
						return *zptr;
					if ((*zptr = alloc_zone (drive)) != 0)
						bio_MARK_MODIFIED (&(bio), tmp);
					temp_zone = *zptr;
					goto new_zone;
				}
				else
				{
					if (!flag || !(*zptr2 = alloc_zone (drive)))
						return 0;
					bio_MARK_MODIFIED (&(bio), tmp2);
					tmp = cput_zone (*zptr2, drive);
					zptr = &((bufr *) tmp->data)->bind[numr & (NR_INDIRECTS2 - 1)];
					temp_zone = *zptr = alloc_zone (drive);
					goto new_zone;
				}
			}
			else
			{
				if (!flag || !(*zptr3 = alloc_zone (drive)))
					return 0;
				bio_MARK_MODIFIED (&(bio), tmp3);
				tmp2 = cput_zone (*zptr3, drive);
				zptr2 = &((bufr *) tmp2->data)->bind[(numr >> LNR_IND2) & (NR_INDIRECTS2 - 1)];
				if (!(*zptr2 = alloc_zone (drive)))
					return 0;
				tmp = cput_zone (*zptr2, drive);
				zptr = &((bufr *) tmp->data)->bind[numr & (NR_INDIRECTS2 - 1)];
				temp_zone = *zptr = alloc_zone (drive);
				goto new_zone;
			}
		}
		if (!flag || !(rip->i_zone[9] = alloc_zone (drive)))
			return 0;
		
		tmp3 = cput_zone (rip->i_zone[9], drive);
		zptr3 = &((bufr *) tmp3->data)->bind[numr >> (LNR_IND2 * 2)];
		if (!(*zptr3 = alloc_zone (drive)))
			return 0;
		
		tmp2 = cput_zone (*zptr3, drive);
		zptr2 = &((bufr *) tmp2->data)->bind[(numr >> LNR_IND2) & (NR_INDIRECTS2 - 1)];
		if (!(*zptr2 = alloc_zone (drive)))
			return 0;
		
		tmp = cput_zone (*zptr2, drive);
		zptr = &((bufr *) tmp->data)->bind[numr & (NR_INDIRECTS2 - 1)];
		temp_zone = *zptr = alloc_zone (drive);
		goto new_zone;
	}
	return 0;
	
new_zone:
	if (temp_zone && flag > 1)
	{
	    tmp = cput_zone (temp_zone, drive);
	}
	return temp_zone;
}

/* This reads zone number 'numr' of an inode .
 * It returns the actual number of valid characters in 'numr' , this is only
 * used for directories so it is hard-coded for the system cache. 
 */

int
next_zone (d_inode *rip, long numr)
{
	register long ret = rip->i_size - numr * BLOCK_SIZE;

	ret = MIN (ret, BLOCK_SIZE);
	
	if (ret < 0) return 0;
	return (int) ret;
}

/* l_write is used internally for doing things a normal user cannot such
 * as writing to a directory ... it accepts 5 parameters , an inode num
 * a position (current position of write) a count which is the number of
 * characters to write,a buffer and a drive , it updates i_size as needed 
 * and allocates zones as required , it is nastier than a read because it 
 * has to write partial blocks within valid blocks and to write beyond EOF
 */

long
l_write (ushort inum, long pos, long len, const void *buf, int drive)
{
	register const char *p = buf;	/* Current position in buffer */
	d_inode rip;
	long chunk;
	long left = len;
	long zne;
	
	read_inode (inum, &rip, drive);
	
	if (pos == -1L) pos = rip.i_size; /* If pos == -1 append */
	chunk = pos >> L_BS;
	
	while (left)	/* Loop while characters remain to be written */
	{
		long zoff;
		ushort wleft;
		UNIT *cp;
	
		zoff = pos & (BLOCK_SIZE - 1);		/* Current zone position */
		wleft = MIN (BLOCK_SIZE - zoff, left);	/* Left to write in curr blk */	
		
		if (zoff || ((left < BLOCK_SIZE) && (pos + left < rip.i_size)))
		{
			zne = find_zone (&rip, chunk++, drive, 2);	/* Current zone in file */
			if (zne == 0) break;				/* Partition full */
			cp = cget_zone (zne, drive);
			bio_MARK_MODIFIED (&(bio), cp);
		}
		else 
		{
			zne = find_zone (&rip, chunk++, drive, 1);	/* Current zone in file */
			if (zne == 0) break;				/* Partition full */
			cp = cput_zone (zne, drive);
		}
		bcopy (p, &((bufr *) cp->data)->bdata[zoff], (ulong) wleft);
		pos += wleft;
		p += wleft;
		if (pos > rip.i_size) rip.i_size = pos;
		left -= wleft;
	}
	
	rip.i_mtime = CURRENT_TIME;
	write_inode (inum, &rip, drive);
	
	return len - left;
}
