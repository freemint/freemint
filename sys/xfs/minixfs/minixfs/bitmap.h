
# ifndef _bitmap_h
# define _bitmap_h

# include "global.h"


long	count_bits	(ushort *buf, long num);

long	alloc_zone	(ushort drive);
long	free_zone	(ushort drive, long zone);

ushort	alloc_inode	(ushort drive);
long	free_inode	(ushort drive, ushort inum);


# endif /* _bitmap_h */
