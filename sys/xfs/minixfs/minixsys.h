
# ifndef _minixsys_h
# define _minixsys_h

# include "global.h"


extern FILESYS minix_filesys;


void sync_bitmaps (register SI *psblk);

# define WB_CHECK(super) (BIO_WB_CHECK (super->di))

INLINE void
sync (ushort dev)
{
	register SI *psblk = super_ptr[dev];
	
	if (!WB_CHECK (psblk))
	{
		/* must sync :-( */
		sync_bitmaps (psblk);
		bio.sync_drv (psblk->di);
	}
}


# endif /* _minixsys_h */
