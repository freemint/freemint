
# ifndef _common_h
# define _common_h

# include "global.h"
# include "io.h"


int	comma_parse	(char *str, llist **list);

FASTFN void	read_zone	(long zone, void *buff);
FASTFN void	write_zone	(long zone, void *buff);
FASTFN int	chk_range	(long zone);
FASTFN void	setbit		(long zone, ushort *map);
FASTFN void	clrbit		(long zone, ushort *map);
FASTFN int	isset		(long zone, ushort *map);

long	findbit		(ushort *map, long limit);
long	alloc_zone	(void);
int	mark_zone	(long zone);
void	unmark_zone	(long zone);
int	ask		(char *str, char *alt);
void	fatal		(char *str);
void	sfatal		(char *str);
void	read_tables	(void);
int	do_trunc	(void);

FASTFN int	chk_irange	(ushort inum);
FASTFN void	inerr		(char *name);
FASTFN int	badname		(char *name);
FASTFN void	cpdir		(dir_struct *dest, dir_struct *src);

void 	showinfo	(void);


FASTFN void
read_zone (long zone, void *buff)
{
	read_blocks (zone, 1, buff);
}

FASTFN void
write_zone (long zone, void *buff)
{
	if (zone == 0)
	{
		printf ("Illegal Write Zone\n");
		return;
	}
	write_blocks (zone, 1, buff);
}

/* Check a zone number is legitimate
 */
FASTFN int
chk_range (long zone)
{
	if ((zone > maxzone) || (zone < minzone))
		return 0;
	
	return 1;
}


/* Bitmap stuff
 */

FASTFN void
setbit (long zone, ushort *map)
{
	map [zone >> 4] |= (1 << (zone & 0xf));
}

FASTFN void
clrbit (long zone, ushort *map)
{
	map [zone >> 4] &= ~(1 << (zone & 0xf));
}

FASTFN int
isset (long zone, ushort *map)
{
	return ((map [zone >> 4] & (1 << (zone & 0xf))) ? 1 : 0);
}


FASTFN int
chk_irange (ushort inum)
{
	if (inum && inum <= maxino)
		return 1;
	
	return 0;
}


FASTFN void
inerr (char *name)
{
	printf ("Inode %ld: %s", cino, name);
}

FASTFN int
badname (char *name)
{
	ushort n = MMAX_FNAME (incr);
	
	if (!*name)
		return 1;
	
	while (n-- && *name)
		if (*name++ == '/')
			return 1;
	
	return 0;
}

/* Copy a directory entry
 */
FASTFN void
cpdir (dir_struct *dest, dir_struct *src)
{
	bcopy (src, dest, DSIZE * incr);
}


# endif /* _common_h */
