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

# include "inode.h"

# include "bitmap.h"
# include "zone.h"


/* Inode routines */

/*
 * Read an inode from the cache
 */

int
read_inode (unsigned int num, d_inode *rip, ushort drv)
{
	SI *psblk = super_ptr[drv];
	UNIT *u;
	
	num -= 1;
	
	u = bio.read (psblk->di, num / psblk->ipb + psblk->ioff, BLOCK_SIZE);
	if (u)
	{
		bufr *tmp = (bufr *) u->data;
		*rip = tmp->binode[num % psblk->ipb];
		
		return 0;
	}
	bzero (rip, sizeof (*rip));
	return -1;
}

/*
 * Write out an inode
 */

int
write_inode (unsigned int num, d_inode *rip, ushort drv)
{
	SI *psblk = super_ptr[drv];
	UNIT *u;
	
	num -= 1;
	
	u = bio.read (psblk->di, num / psblk->ipb + psblk->ioff, BLOCK_SIZE);
	if (u)
	{
		bufr *tmp = (bufr *) u->data;
		tmp->binode[num % psblk->ipb] = *rip;
		
		bio_MARK_MODIFIED (&(bio), u);
		return 0;
	}
	return -1;	
}

/* Truncate an inode to 'count' zones, this is used by unlink() as well as
 * (f)truncate() . Bit tricky this , we have to note which blocks to free,
 * and free indirection/double indirection blocks too but iff all the blocks
 * inside them are free too. We also need to keep count of how much to leave 
 * alone, sparse files complicate this a bit .... so do 2 fs versions ....
 */

void
trunc_inode (d_inode *rip, ushort drive, long count, int zap)
/* count - number of blocks to leave */
/* zap - flag to alter inode */
{
	SI *psblk = super_ptr[drive];
	UNIT *q;
	bufr *tmp = NULL;
	int i, j;
	char some, dirty;
	
	/* Handle zones in inode first */
	if (count < psblk->dzpi)
	{
		for (i = count; i < psblk->dzpi; i++)
		{
			if (rip->i_zone[i])
			{
				free_zone (drive, rip->i_zone[i]);
			}
			if (zap)
			{
				rip->i_zone[i] = 0;
			}
		}
		count = 0;
	}
	else
	{
		count -= psblk->dzpi;
	}

	/* Handle indirect zone */
	if (count < psblk->zpind)
	{
		some = 0;
		dirty = 0;
		if (rip->i_zone[7])
		{
			q = cget_zone (rip->i_zone[7], drive);
			tmp = (bufr *) q->data;
			for (i = 0; i < psblk->zpind; i++)
			{
				if (PIND (tmp, i))
				{
					if (count) some = 1;
					else
					{
						free_zone (drive, PIND (tmp, i));
						if (zap)
						{
							PIND (tmp, i) = 0;
						}
						dirty = 1;
					}
				}	
				if (count)
					count--;
			}
			if (!some)
			{
				free_zone (drive, rip->i_zone[7]);
				if (zap)
					rip->i_zone[7] = 0;
			}
			else if (dirty)
			{
# if 1
				/* a bit faster :) */
				bio_MARK_MODIFIED (&(bio), q);
# else
				write_zone (rip->i_zone[7], tmp, drive);
# endif
			}
		}
	}
	else
		count -= psblk->zpind;
	
	/* Handle double indirect ... */
	if (count < (long) psblk->zpind * psblk->zpind)
	{
		bufr temp;
		
		if (rip->i_zone[8])
		{
			some = 0;
			dirty = 0;
			
			read_zone (rip->i_zone[8], &temp, drive);
			for (i = 0; i < psblk->zpind; i++)
			{
				if (IND (temp, i))
				{
					char lsome = 0; /* local some, dirty for inds */
					char ldirty = 0;
					
					q = cget_zone (IND (temp, i), drive);
					tmp = (bufr *) q->data;
					for (j = 0; j < psblk->zpind; j++)
					{
						if (PIND (tmp, j))
						{
							if (count)
							{ 
								some = 1;
								lsome = 1;
							}
							else
							{
								free_zone (drive, PIND (tmp, j));
								if (zap)
								{
									PIND (tmp, j) = 0;
								}
								ldirty = 1;
							}
						}
						if (count)
							count--;
			    		}
			    		if (!lsome)
			    		{
						free_zone (drive, IND (temp, i));
						if (zap)
						{
							IND (temp, i) = 0;
						}
						dirty = 1;
			    		}
			    		else if (ldirty)
			    		{
# if 1
						bio_MARK_MODIFIED (&(bio), q);
# else
						write_zone (IND (temp, i),tmp, drive);
# endif
					}
				}
				else 
				{
					if (count >= psblk->zpind) count -= psblk->zpind;
					else count = 0;
				}
			}
			if (!some)
			{
				free_zone (drive, rip->i_zone[8]);
				if (zap) rip->i_zone[8] = 0;
			}
			else if (dirty) write_zone (rip->i_zone[8], &temp, drive);
		}
	}
	else
	{
		count -= (long) psblk->zpind * psblk->zpind;
	}
	
	/* Handle triple indirect ... */
	if (rip->i_zone[9])
	{
		bufr temp;
		
		some = 0;
		dirty = 0;
		
		read_zone (rip->i_zone[9], &temp, drive);
		for (i = 0; i < psblk->zpind; i++)
		{
			if (IND (temp, i))
			{
				bufr temp2;
				char lsome = 0; /* local some, dirty for inds */
				char ldirty = 0;
				
				read_zone (IND (temp, i), &temp2, drive);
				for (j = 0; j < psblk->zpind; j++)
				{
					if (IND (temp2, j))
					{
						UNIT *u;
						bufr *tmp2;
						char lsome1 = 0;
						char ldirty1 = 0;
						int k;
						
						u = cget_zone (IND (temp2, j), drive);
						tmp2 = (bufr *) u->data;
						for (k = 0; k < psblk->zpind; k++)
						{
							if (PIND (tmp2, k))
							{
								if (count)
								{
									some = 1;
									lsome = 1;
									lsome1 = 1;
								}
								else
								{
									free_zone (drive, PIND (tmp2, k));
									if (zap)
									{
										PIND (tmp2, k) = 0;
									}
									ldirty1 = 1;
								}
							}
							if (count)
								count--;
						}
						if (!lsome1)
						{
							free_zone (drive, IND (temp2, j));
							if (zap)
							{
								PIND (tmp, j) = 0;
							}
							ldirty = 1;
						}
						else if (ldirty1)
						{
							bio_MARK_MODIFIED (&(bio), u);
						}
					}
					else
					{
						if (count >= psblk->zpind) count -= psblk->zpind;
						else count = 0;
					}
				}
				if (!lsome)
				{
					free_zone (drive, IND (temp, i));
					if (zap)
					{
						IND (temp, i) = 0;
					}
					dirty = 1;
				}
				else if (ldirty)
				{
					write_zone (IND (temp, i), &temp2, drive);
				}
			}
			else 
			{
				if (count >= (long) psblk->zpind * psblk->zpind) count -= (long) psblk->zpind * psblk->zpind;
				else count = 0;
			}
		}
		if (!some)
		{
			free_zone (drive, rip->i_zone[9]);
			if (zap)
			{
				rip->i_zone[9] = 0;
			}
		}
		else if (dirty)
		{
			write_zone (rip->i_zone[9], &temp, drive);
		}
	}
}

/* Inode version of (f)truncate, truncates a file to size 'length'
 */

long
itruncate (unsigned int inum, ushort drive, long int length)
{
	long count;
	d_inode rip;
	FILEPTR *p;
	
	read_inode (inum,&rip,drive);
	
	/* Regulars only , clever directory compaction stuff later ... */
	if (!IS_REG (rip)) return EACCES;
	
	/* If file smaller than 'length' nothing to do */
	if (rip.i_size <= length)
	{
# if 0
		rip.i_size = length;
# endif
		return 0;
	}
	count = (length + 1023) / 1024;
	
	/* invalidate f_cache zones */
	for (p = firstptr; p; p = p->next)
	{
		if ((drive == p->fc.dev)
			&& (inum == p->fc.index)
			&& (((f_cache *) p->devinfo)->lzone > count))
		{
			((f_cache *) p->devinfo)->lzone = 0;
		}
	}
	
	trunc_inode (&rip, drive, count, 1);
	rip.i_size = length;	
	
	write_inode (inum, &rip, drive);
	
	return 0;
}
