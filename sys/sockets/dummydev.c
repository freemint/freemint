/*
 *	Skeleton for the info devices (/dev/unix, /dev/route, ...)
 *
 *	02/28/94, kay roemer.
 */

# include "dummydev.h"

# include "mint/dcntl.h"
# include "mint/ioctl.h"
# include "mint/pathconf.h"


long
dummydev_init (char *name, struct dev_descr *descr)
{
	long r;
	
	r = d_cntl (DEV_INSTALL, name, (long) descr);
	if (!r || r == ENOSYS)
	{
		char message[128];
		
		ksprintf (message, "Cannot install device %s\r\n", name);
		c_conws (message);
		
		return -1;
	}
	
	return 0;
}	

long
dummydev_open (FILEPTR *f)
{
	/* Nothing to do */
	return 0;
}

long
dummydev_write (FILEPTR *f, const char *buf, long nbytes)
{
	return EACCES;
}

long
dummydev_lseek (FILEPTR *f, long where, int whence)
{
	switch (whence)
	{
		case SEEK_SET:
			f->pos32 = where;
			return f->pos32;
		
		case SEEK_CUR:
			f->pos32 += where;
			return f->pos32;
		
		case SEEK_END:
			return EACCES;
	}
	
	return ENOSYS;
}

long
dummydev_ioctl (FILEPTR *f, int mode, void *buf)
{
	switch (mode)
	{
		case FIONREAD:
			*(long *) buf = UNLIMITED;
			return 0;
		
		case FIONWRITE:
			*(long *) buf = UNLIMITED;
			return 0;
		
		default:
			return ENOSYS;
	}
}

long
dummydev_datime (FILEPTR *f, ushort *timeptr, int rwflag)
{
	if (!rwflag)
	{
		timeptr[0] = t_gettime ();
		timeptr[1] = t_getdate ();
	}
	
	return 0;
}

long
dummydev_close (FILEPTR *f, int pid)
{
	/* Nothing to do */
	return 0;
}

long
dummydev_select (FILEPTR *f, long proc, int mode)
{
	return 1;
}

void
dummydev_unselect (FILEPTR *f, long proc, int mode)
{
	/* Nothing to do */
}
