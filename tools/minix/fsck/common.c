/* This file is part of 'fsck' Copyright S.N. Henson
 */

/* Functions Common to V1 and V2 Filesystems
 */

# include "common.h"


/* Parse a comma separated list of integers, return number of integers read.
 * Fill in elements in a linked list. 
 */

int
comma_parse (char *str, llist **list)
{
	char *tstr;
	char *fstr;
	char *comma;
	int count = 0;
	
	tstr = strdup (str);
	if (!tstr)
		fatal ("Out of memory");
	
	fstr=tstr;
	do
	{
		llist *p;
		
		comma = strchr(tstr,',');
		if (comma)
			*comma = 0;
		
		p = malloc (sizeof (*p));
		if (!p)
			fatal ("Out of memory");
		
		p->member = atol (tstr);		
		p->next = *list;
		*list = p;
		count++;
		
		if (comma)
			tstr = ++comma;
	}
	while (comma);
	
	free (fstr);
	
	return count;
}

/* Find the first Zero bit in bitmap
 */
long
findbit (ushort *map, long limit)
{
	long ret;
	long i;
	int j;

	for (i = 0; i < (limit + 15) / 16; i++)
	{
		if (map[i] != 0xffff)
		{
			for (j = 0; j < 16; j++) 
			{
				if (!(map[i] & (1 << j)))
				{
							
					ret = i * 16 + j;
					if (ret < limit)
					{
						setbit (ret, map);
						return ret;
					}
					return 0;
				}
			}
		}
	}
	return 0;
}

long
alloc_zone (void)
{
	long ret;
	ret = findbit (szbitmap, maxzone - minzone);
	if (!ret) return 0;
	ret += minzone - 1;
	return ret;
}

int
mark_zone (long zone)
{
	ushort ret;
	zone -= minzone - 1;
	ret = isset (zone, szbitmap);
	setbit (zone, szbitmap);
	return ret;
}

void
unmark_zone (long zone)
{
	zone -= minzone - 1;
	clrbit (zone, szbitmap);
}

int
ask (char *str, char *alt)
{
	char ans [20];
	
	if (alln)
		return 0;
	
	if (ally)
	{
		printf ("(%s)\n", alt);
		return 1;
	}
	
	printf ("%s", str);
	
	fgets (ans, 20, stdin);
	if ((ans [0] & ~32) == 'Y')
		return 1;
	
	return 0;
}

void
fatal (char *str)
{
	printf ("Fatal error: %s\n", str);
	close_device ();
	exit (1);
}

void
sfatal (char *str)
{
	printf ("Bad Superblock: %s\n", str);
	close_device ();
	exit (1);
}


/* Read in superblock, perform some sanity checks on it, then read in the 
 * bitmaps.
 */

void
read_tables (void)
{
	long expect;
	
	Super = malloc (BLOCKSIZE);
	if (!Super)
		fatal ("out of memory");
	
	read_zone (1, Super);
	
	/* Sanity checks on super block */
	if (Super->s_magic != SUPER_V1 && Super->s_magic != SUPER_V1_30)
	{
		if (Super->s_magic == SUPER_V2) 
		{
			version = 1;
			maxzone = Super->s_zones;
		}
		else
			sfatal ("Invalid Magic Number");
	}
	else
		maxzone = Super->s_nzones;
	
	
	if (set_size (maxzone))
		fatal ("Cannot access filesystem");
	
	
	expect = (Super->s_ninodes + 8192l) / 8192;
	
	if (expect > Super->s_imap_blks)
		sfatal("Inode Bitmap too Small");
	
	if (expect < Super->s_imap_blks)
		fprintf (stderr, "Warning: inode bitmap %d blocks, expected %ld\n",
			Super->s_imap_blks, expect);
	
	
	expect = (maxzone + 8191) / 8192;
	
	if (expect > Super->s_zmap_blks)
		sfatal ("Zone Bitmap too Small");
	
	if (expect < Super->s_zmap_blks)
		fprintf (stderr, "Warning: zone bitmap %d blocks, expected %ld\n",
			Super->s_zmap_blks, expect);
	
	
	/* Set up some variables
	 */
	maxino = Super->s_ninodes;
	minzone = Super->s_firstdatazn;
	maxzone--;
}

int
do_trunc (void)
{
	printf ("Inode %ld Contains Too Many Zones\n",cino);
	done_trunc = 1;
	if (ask ("Truncate?", "Truncated"))
	{
		trunc = 1;
		return 1;
	}
	return 0;
}

/* Show FS info
 */
void
showinfo (void)
{
	if (info)
	{
		printf ("\nDrive %s\n", drvnam);
		printf ("V%d Filesystem\n", version + 1);
		printf ("Directory Increment %d\n", incr);
		printf ("%ld\t\tInodes\n", maxino);
		printf ("%ld\t\tFree Inodes\n", ifree);
		printf ("%ld\t\tData Zones\n", maxzone - minzone + 1);
		printf ("%ld\t\tFree Data Zones\n", zfree);
		printf ("%ld\t\tDirectories\n", ndir);
		printf ("%ld\t\tRegular Files\n", nreg);
		printf ("%ld\t\tSymbolic Links\n\n", nsym);
		printf ("%ld\t\tFIFO's\n", nfifo);
		printf ("%ld\t\tCharacter Special Files\n", nchr);
		printf ("%ld\t\tBlock Special Files\n", nblk);
	}

	if (info > 1)
	{
		printf ("First Inode Block  %ld\n", ioff);
		printf ("First Datazone     %ld\n\n", minzone);
	}
	
	if (modified) 
	{
		printf ("**************************************\n");
		printf ("*    FILESYSTEM HAS BEEN MODIFIED    *\n");
		printf ("**************************************\n\n");
	}
}
