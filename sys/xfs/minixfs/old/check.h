
# ifndef _check_h
# define _check_h

# include "minix.h"

void	chk_zone	(long start, int count, int drive);
void	chk_iblock	(long start, super_info *psblk);

# endif /* _check_h */
