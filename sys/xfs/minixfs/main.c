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

# include <mintbind.h>

# include "main.h"

# include "minixsys.h"
# include "inode.h"
# include "zone.h"
# include "version.h"


/*
 * Initialisation routine called first by the kernel
 */

FILESYS *
init (struct kerinfo *k)
{
	kernel = k;
	
	c_conws ("\033p Minix file system driver for MiNT. \033q\r\n");
	c_conws ("Version " str (MFS_MAJOR) "." str (MFS_MINOR) "\r\n");
	c_conws ("Copyright 1991,1992,1993,1994,1995 S.N.Henson\r\n");
# ifdef MFS_PLEV
	c_conws ("Patch Level " str (MFS_PLEV) "\r\n");
# endif
# ifdef PRE_RELEASE
	c_conws ("Pre-Release Test Version\r\n");
# endif
	c_conws ("\033pThis is a modified binary version for FreeMiNT 1.15.\033q\r\n");
	c_conws ("� " __DATE__ " by Frank Naumann.\r\n\r\n");
	
	/* check MiNT version */
	if ((k->maj_version < 1) || (k->maj_version == 1 && k->min_version < 15))
	{
		c_conws ("\033pThis version requires at least FreeMiNT 1.15!\033q\r\n");
		c_conws ("\7Minix-FS not installed.\r\n\r\n");
		return NULL;
	}
	
	/* check buffer cache revision */
	if (bio.version != 3)
	{
		c_conws ("\033pWrong MiNT buffer cache version!\033q\r\n");
		c_conws ("\7Minix-FS not installed.\r\n\r\n");
		return NULL;		
	}
	
	/* check for revision 1 features */
	if (bio.revision < 1)	
	{
		c_conws ("\033pFreeMiNT buffer cache revision to old!\033q\r\n");
		c_conws ("\7Minix-FS not installed.\r\n\r\n");
		
		return NULL;		
	}
	
	error.data = kmalloc (BLOCK_SIZE);
	if (!error.data)
	{
		c_conws ("\033pCan't alloc error unit - out of memory?\033q\r\n");
		c_conws ("\7Minix-FS not installed.\r\n\r\n");
		return NULL;
	}
	
	/* check for native UTC timestamps */
	if (MINT_KVERSION > 0 && KERNEL->xtime)
	{
		/* yeah, save enourmous overhead */
		native_utc = 1;
		DEBUG (("Minix-FS: running in native UTC mode!"));
	}
	else
	{
		/* disable extension level 3 */
		minix_filesys.fsflags &= ~FS_EXT_3;
	}
	
	DEBUG (("Minix-FS: loaded and ready (k = %lx).", k));
	return &minix_filesys;
}

/* Sanity checker, checks a filesystem is minix and then sets up all internal
 * structures accordingly, e.g. superblock and directory increment. Returns
 * E_OK if the filesystem is minix and valid, negative error number otherwise.
 */

int
minix_sanity (int drv)
{
	UNIT *u;
	SB *sblk;
	DI *di = NULL;
	SI *psblk = NULL;
	long r;
	
	DEBUG (("Minix-FS (%c): minix_sanity enter.", drv+'A'));
	
	psblk = kmalloc (sizeof (*psblk));
	if (!psblk)
	{
		ALERT (("Minix-FS (%c): No memory for super_info structure.", drv+'A'));
		
		r = ENOMEM;
		goto failure;
	}
	
	bzero (psblk, sizeof (*psblk));
	
	di = bio.get_di (drv);
	if (!di)
	{
		DEBUG (("Minix-FS (%c): Cannot access partition.", drv+'A'));
		
		r = EBUSY;
		goto failure;
	}
	
	if (di->pssize != 512 && di->pssize != 1024)
	{
		DEBUG (("Minix-FS (%c): Cannot access partition with sectorsize > 1024.", drv+'A'));
		
		r = ENXIO;
		goto failure;
	}
	
	bio.set_lshift (di, BLOCK_SIZE);
	
	super_ptr[drv] = psblk;
	psblk->di = di;
	psblk->s_flags = 0;
	
	u = bio.read (di, SUPER_BLOCK, BLOCK_SIZE);
	if (!u)
	{
		DEBUG (("Minix-FS (%c): bio.read fail.", drv+'A'));
		
		r = EIO;
		goto failure;
	}
	
	sblk = (SB *) u->data;
	
	if ((sblk->s_magic == SUPER_V2) && sblk->s_ninodes)
	{
		/* read in superblock and stay resident */
		u = bio.get_resident (di, SUPER_BLOCK, BLOCK_SIZE);
		if (!u)
		{
			DEBUG (("Minix-FS (%c): bio.get_resident fail.", drv+'A'));
			
			r = ENOMEM;
			goto failure;
		}
		
		sblk = (SB *) u->data;
		
		psblk->sunit = u;
		psblk->sblk = sblk;	
	}
	else
	{
		if ((sblk->s_magic == SUPER_V1_30) || (sblk->s_magic == SUPER_MAGIC))
		{
			ALERT (("Minix-FS (%c): V1 filesystem are no longer supported.", drv+'A'));
			r = EMEDIUMTYPE;
		}
		else
		{
			DEBUG (("Minix-FS (%c): invalid MAGIC in 1.", drv+'A'));
			r = ENXIO;
		}
		
		goto failure;
	}
	
	if ((sblk->s_magic == SUPER_V2) && sblk->s_ninodes)
	{
		d_inode rip;
		long maps;
		
		/* default return */
		r = EMEDIUMTYPE;
		
		if (sblk->s_log_zsize)
		{
			ALERT (("Minix-FS (%c): Cannot read Drive, Zone-size > Block-size", drv+'A'));
			goto failure;
		}
		
		DEBUG (("Minix-FS (%c): V2 filesystem", drv+'A'));
		
		psblk->ipb = INODES_PER_BLOCK2;
		psblk->zpind = NR_INDIRECTS2;
		psblk->dzpi = NR_DZONE_NUM2;
		psblk->ndbl = NR_DBL2;
		
		maps = sblk->s_imap_blks + sblk->s_zmap_blks;
		psblk->ioff = maps + 2;
		
		read_inode (ROOT_INODE, &rip, drv);
		if (IS_DIR (rip))
		{
			bufr temp;
			char *p;
			int dot = -1;
			int dotdot = -1;
			int i;
			
			DEBUG (("Minix-FS (%c): maps = %li -> %li bytes", drv+'A', maps, maps * BLOCK_SIZE));
			
			maps *= BLOCK_SIZE;
			p = kmalloc (maps);
			if (!p)
			{
				ALERT (("Minix-FS (%c): No memory for bitmaps!", drv+'A'));
				
				r = ENOMEM;
				goto failure;
			}
			
			psblk->ibitmap = (void *) p;
			psblk->zbitmap = (void *) (p + BLOCK_SIZE * sblk->s_imap_blks);
			
			r = BIO_RWABS (di, 2, p, maps, 2);
			if (r)
			{
				ALERT (("Minix-FS (%c): Bitmaps read failure (%li), access read-only!", drv+'A'));
				psblk->s_flags |= MS_RDONLY;
			}
			
			psblk->idirty = 0;
			psblk->zdirty = 0;
			psblk->zlast = 0;
			psblk->ilast = 0;
			
			/* Final step , read in the root directory zone 1 and
			 * check the '.' and '..' spacing , The spacing
			 * between the '.' and '..' will be used as an
			 * indicator of the directory entry size. If in doubt
			 * assume a normal minix filesystem.
			 */
			read_zone (rip.i_zone[0], &temp, drv);
			
			for (i = 0; i < MIN (NR_DIR_ENTRIES, rip.i_size / DIR_ENTRY_SIZE); i++)
			{
				if (temp.bdir[i].d_inum)
				{
					if (!strcmp (temp.bdir[i].d_name, "."))
					{
						if (dot == -1)
						{
							dot = i;
						}
						else
						{
							ALERT (("Minix-FS (%c): multiple \".\" in root dir!", drv+'A'));
							dot = -1;
							i = NR_DIR_ENTRIES;
						}
					}

					if (!strcmp (temp.bdir[i].d_name, ".."))
					{
						if (dotdot == -1)
						{
							dotdot = i;
						}
						else
						{
							ALERT (("Minix-FS (%c): multiple \"..\" in root dir!", drv+'A'));
							dotdot = -1;
							i = NR_DIR_ENTRIES;
						}
					}
				}
			}
			
			if ((dotdot == -1) || (dot == -1))
			{
				ALERT (("Minix-FS (%c): no '.' or '..' in root dir", drv+'A'));
				goto failure;
			}
			
			psblk->incr = dotdot - dot;
			if ((psblk->incr < 1) || (psblk->incr > 16) || NPOW2 (psblk->incr))
			{
				ALERT (("Minix-FS (%c): weird '.' '..' positions", drv+'A'));
				goto failure;
			}
			
			psblk->mfname = MMAX_FNAME (psblk->incr);
			
			if (!(psblk->sblk->s_state & MINIX_VALID_FS)
				|| (psblk->sblk->s_state & MINIX_ERROR_FS))
			{
				psblk->s_flags |= S_NOT_CLEAN_MOUNTED;
			}
			
			/* setup write protection */
			if (BIO_WP_CHECK (di))
				psblk->s_flags |= MS_RDONLY;
			
			if (!(psblk->s_flags & MS_RDONLY))
			{
				if (!(psblk->sblk->s_state & MINIX_VALID_FS))
				{
					ALERT (("MinixFS [%c]: WARNING: mounting unchecked fs, running fsck is recommended", drv+'A'));
				}
				else if (psblk->sblk->s_state & MINIX_ERROR_FS)
				{
					ALERT (("MinixFS [%c]: WARNING: mounting fs with errors, running fsck is recommended", drv+'A'));
				}
				
				psblk->sblk->s_state &= ~MINIX_VALID_FS;
				bio_MARK_MODIFIED (&bio, psblk->sunit);
			}
			
			DEBUG (("Minix-FS (%c): minix_sanity leave E_OK (incr = %i, mfname = %i).", drv+'A', psblk->incr, psblk->mfname));
			return E_OK;
		}
		else
		{
			ALERT (("Minix-FS (%c): root inode is not a directory?", drv+'A'));
			goto failure;
		}
	}
	else
	{
		DEBUG (("Minix-FS (%c): invalid MAGIC.", drv+'A'));
		r = ENXIO;
	}
	
failure:
	if (psblk)
	{
		if (psblk->ibitmap)
			kfree (psblk->ibitmap);
		
		kfree (psblk);
	}
	
	if (di) bio.free_di (di);
	super_ptr[drv] = NULL;
	
	DEBUG (("Minix-FS (%c): minix_sanity leave r = %li", drv+'A', r));
	return r;
}
