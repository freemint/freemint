
# ifndef _dir_h
# define _dir_h

# include "global.h"


long	search_dir	(const char *name, unsigned inum, ushort drive, int flag);
long	is_parent	(unsigned dir1, unsigned dir2, ushort drive);


# endif /* _dir_h */
