
# ifndef _minixdev_h
# define _minixdev_h

# include "global.h"


extern DEVDRV minix_dev;

long	m_close		(FILEPTR *f, int pid);


# endif /* _minixdev_h */
