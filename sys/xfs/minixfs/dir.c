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

# include "dir.h"

# include "inode.h"
# include "trans.h"
# include "zone.h"


INLINE int	badname		(char *name);

/* Return '1' if 'name' has any illegal characters in it */

INLINE int
badname (char *name)
{
	if (!*name)
	{
		DEBUG (("Minix-FS: Illegal null filename"));
		return 1;
	}
	
	for (; *name; name++)
	{
		if (BADCHR (*name)) 
		{
			DEBUG (("Minix-FS: Bad character in filename"));
			return 1;
		}
	}
	
	return 0;
}


/*
 * search_dir serves 3 functions dependent on 'mode'
 * 0 Search a directory for the entry 'name' if found return its inode number.
 * 1 Create an entry 'name' return position of d_inum .
 * 2 Delete an entry 'name' return inode num for success .
 * 3 Find entry 'name', return position of d_inum.
 * In all cases failure is denoted by a negative error number .
 */

long
search_dir (const char *name, unsigned inum, ushort drive, int flag)
{
	int entry, count;
	long zone;		/* Zone number within dir */
	long lstfree;		/* Last unused dir entry */
	d_inode rip;
	
	int tflag = do_trans (SRCH_TOS, drive);
	int incr = super_ptr[drive]->incr;
	int mfname = super_ptr[drive]->mfname;
	
	char bname[mfname + 8];
	char tname[mfname + 8];
	
	
	if (tflag)
		strcpy (tname, tosify (name, tflag, bname, mfname));
	
	read_inode (inum, &rip, drive);
	
	/* Must be a directory ! */
	if (!IS_DIR (rip)) return ENOTDIR;
	
	lstfree = -1L;
	for (zone = 0; (count = next_zone (&rip, zone) >> L_DIR_SIZE); zone++)
	{
		UNIT *u = cget_zone (find_zone (&rip, zone, drive, 0), drive);
		
		dir_struct *try;
		for (entry = 0, try = ((bufr *) u->data)->bdir; entry < count; entry += incr, try += incr)
		{
			ushort inumtemp = try->d_inum;
			
			if (inumtemp &&
				(!strncmp (name, try->d_name, mfname)
				|| (tflag &&  (!strcmp (tname, tosify (try->d_name, tflag, bname, mfname))))))
			{
				if (flag == KILL)
				{
					/* Never ever allow unlinking of '.' or '..' */
					if (zone == 0 && entry < (2 * incr))
						return EACCES;
					
					try->d_inum = 0;

					/* Next bit pinched from Minix,
					 * store old inode num in last 2 bytes of name
					 * This allows recovery in case of accident
					 */
					{
						ushort *ptr;
						ptr = (ushort *)&try->d_name[mfname - 2];
						*ptr = inumtemp;
					}
					
					bio_MARK_MODIFIED (&(bio), u);
				}
				if (flag == ADD) return EACCES;
				if (flag == FIND || flag == KILL) return inumtemp;
				if (flag == POS) return entry * DIR_ENTRY_SIZE + zone * BLOCK_SIZE;
			}
			if (flag == ADD && lstfree == -1L && !inumtemp)
				lstfree = zone * BLOCK_SIZE + entry * DIR_ENTRY_SIZE;
		}
	}

	if (flag == ADD)
	{
		dir_struct add[incr];
		strncpy (tname, name, mfname);
		tname[mfname] = 0;
		
		if (badname (tname))
			return EACCES;
		
		if (do_trans (LWR_TOS, drive))
			strlwr(tname);
		
      		strncpy (add[0].d_name, tname, mfname);
		add[0].d_inum = 0;
		
		if (l_write (inum, lstfree, (long)(mfname + 2), add, drive) != (mfname + 2))
			return EACCES;
		
		return (lstfree == -1L ? rip.i_size : lstfree);
	}
	
	return ENOENT;
}

/*
 * Return '1' if dir2 is a parent of dir1, otherwise 0 or negative error 
 * number 
 */

long
is_parent (unsigned dir1, unsigned dir2, ushort drive)
{
	long itemp = dir1;
	
	for(;;)
	{
		if (itemp == dir2)
		{
			DEBUG (("Minix-FS (%c): invalid directory move.", DriveToLetter(drive)));
			return 1;
		}
		
		if (itemp == ROOT_INODE)
			break;
		
		itemp = search_dir ("..", itemp, drive, FIND);
		if (itemp < 0)
		{
			ALERT (("Minix-FS (%c): Couldn't trace inode %d back to root.", DriveToLetter(drive), dir1));
			return EACCES;
		}
	}
	
	return 0;
}


