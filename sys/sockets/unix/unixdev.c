/*
 *	This file implements /dev/unix. It is intended for controlling
 *	the behavior of the unix domain layer and getting information
 *	about it. netstat(8) is implemented using this device.
 *
 *	12/15/93, kay roemer.
 */

# include "unixdev.h"

# include "unix.h"

# include "dummydev.h"
# include "util.h"

# include <mint/file.h>


/* read() obtains this structure for every unix domain socket */
struct unix_info
{
	short		proto;	 /* protcol numer, always 0 */
	short		flags;	 /* socket flags, SO_* */
	short		type;	 /* socket type, SOCK_DGRAM or SOCK_STREAM */
	short		state;	 /* socket state, SS_* */
	short		qlen;	 /* bytes in read buffer */
	short		addrlen; /* addrlen, 0 if no address */
	struct sockaddr_un addr; /* addr, only meaningful if addrlen > 0 */
};

static long	unixdev_read	(FILEPTR *, char *, long);

static DEVDRV unixdev =
{
	open:		dummydev_open,
	write:		dummydev_write,
	read:		unixdev_read,
	lseek:		dummydev_lseek,
	ioctl:		dummydev_ioctl,
	datime:		dummydev_datime,
	close:		dummydev_close,
	select:		dummydev_select,
	unselect:	dummydev_unselect
};

static struct dev_descr unixdev_descr =
{
	driver:		&unixdev,
	fmode:		S_IFCHR | S_IRUSR | S_IWUSR
};

static char unixdev_name[] = "u:\\dev\\unix";

long
unixdev_init (void)
{
	return dummydev_init (unixdev_name, &unixdev_descr);
}	

static long
unixdev_read (FILEPTR *fp, char *buf, long nbytes)
{
	struct un_data *unp = 0;
	struct unix_info info, *infop = (struct unix_info *) buf;
	long space;
	
	for (space = nbytes; space >= sizeof (info); ++fp->pos)
	{
		int i, j;
		
		for (i = fp->pos, j = 0; j < UN_HASH_SIZE && i >= 0; ++j)
		{
			unp = allundatas[j];
			for (; unp && --i >= 0; unp = unp->next)
				;
		}
		
		if (j >= UN_HASH_SIZE)
			break;
		
		info.proto	= unp->proto;
		info.flags	= unp->sock->flags;
		info.type	= unp->sock->type;
		info.state	= unp->sock->state;
		info.addrlen	= unp->addrlen;
		info.addr	= unp->addr;
		
		if (info.type == SOCK_DGRAM)
		{
			struct dgram_hdr header = { 0, 0 };
			
			if (UN_USED (unp))
				un_read_header (unp, &header, 0);
			
			info.qlen = header.nbytes;
		}
		else
			info.qlen = UN_USED (unp);
		
		*infop++ = info;
		space -= sizeof (info);
	}
	
	return (nbytes - space);
}
