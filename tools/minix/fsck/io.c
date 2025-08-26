/* The functions read_blocks(start,count,buff) and write_blocks(), read/write
 * 'count' 1K blocks from disk starting at block 'start' into the buffer
 * pointed to by 'buff' . The function init_device(name,rw) initialises the 
 * device 'name' so that read_blocks() and write_blocks() use it. Since this
 * is called once initially, any other machine dependent initialisation can be
 * put here, if it returns non-zero then the operation failed. If 'rw' is not
 * zero, write access is sought also for this device. The function set_size is
 * used to tell the I/O routines the size of the filesystems being accessed,
 * so any special arrangements can be made. If this function returns non-zero
 * then the operation failed and the filesystem cannot be accessed.
 */

# ifdef atarist

# include "io.h"
# include "common.h"
# include "xhdi.h"


static ushort	drv;
static ushort	write_flag;
static ushort	key;
static ushort	major;
static ushort	minor;
static ulong	blocksize;	/* 0 - 1024, 1 - 512 */
static ulong	start;
static ulong	blocks;


void
read_blocks (ulong rec, ushort count, void *buff)
{
	register ulong recno = rec << blocksize;
	register ushort n = count << blocksize;
	
	if (!n)
	{
		printf ("read_blocks: n = 0!");
		fatal ("internal failure");
		return;
	}
	
	if ((recno + n) > blocks)
	{
		printf ("read_blocks: access outside partition!");
		fatal ("internal failure");
		return;
	}
	
	recno += start;
	
	{	register long ret;
		
		ret = XHReadWrite (major, minor, 0, recno, n, buff);
		if (ret)
		{
			printf ("XHReadWrite (read) returned %ld\n", ret);
			fatal ("I/O error");
		}
	}
}
void
write_blocks (ulong rec, ushort count, void *buff)
{
	register ulong recno = rec << blocksize;
	register ushort n = count << blocksize;
	
	if (start == 0)
	{
		printf ("Illegal Write - start = 0\n");
		fatal ("internal failure");
		return;
	}
	
	if (!write_flag)
	{
		printf ("Illegal write - start = %ld, count = %d\n", rec, count);
		fatal ("internal failure");
		return;
	}
	
	if (!n)
	{
		printf ("read_blocks: n = 0!");
		fatal ("internal failure");
		return;
	}
	
	if ((recno + n) > blocks)
	{
		printf ("read_blocks: access outside partition!");
		fatal ("internal failure");
		return;
	}
	
	modified = 1;
	recno += start;
	
	{	register long ret;
		
		ret = XHReadWrite (major, minor, 1, recno, n, buff);
		if (ret)
		{
			printf ("XHReadWrite (write) returned %ld\n", ret);
			fatal ("I/O error");
		}
	}
}


/* Constants for fscntl
 */
# define MFS_BASE	(0x100)
# define MFS_VERIFY	(MFS_BASE)	/* Return minixfs magic number */
# define MFS_MAGIC	(0x18970431)	/* Magic number from MFS_VERIFY */


#define DriveFromLetter(d) \
	(((d) >= 'A' && (d) <= 'Z') ? ((d) - 'A') : \
	 ((d) >= 'a' && (d) <= 'z') ? ((d) - 'a') : \
	 ((d) >= '1' && (d) <= '6') ? ((d) - '1' + 26) : \
	 -1)

int
init_device (char *name, int rw)
{
	char id [4];
	ulong flags;
	long r;
	
	
	/* calc and validate drv number
	 */
	drv = DriveFromLetter(name[0]);
	if (drv > 31)
	{
		fprintf (stderr, "Invalid drive name %s\n", name);
		return 1;
	}
	
	/* get partition data
	 */
	
	r = XHInqDev2 (drv, &major, &minor, &start, NULL, &blocks, id);
	
	if (!r)
		r = XHInqTarget2 (major, minor, &blocksize, &flags, NULL, 0);
	
	if (r)
	{
		fprintf (stderr, "failure while getting partition data\n");
		return 1;
	}
	
	/* check blocksize
	 */
	if (blocksize == 512)
	{
		blocksize = 1;
	}
	else if (blocksize == 1024)
	{
		blocksize = 0;
	}
	else
	{
		fprintf (stderr, "sector size not supported [%ld]\n", blocksize);
		return 1;
	}
	
	if (flags & XH_TARGET_RESERVED)
	{
		fprintf (stderr, "drive reserved\n");
		return 1;
	}
	
	/* lock through XHDI
	 */
	key = XHLock (major, minor, 1, 0);
	
	
	/* set write or read-only mode
	 */
	if (rw)
		write_flag = 1;
	
	
	/* lock the device
	 */
	if (Dlock (1, drv))
	{
		fprintf (stderr, "Couldn't sync or lock device %s.\n", name);
		if (ask ("Continue?", "Ignored"))
		{
			fprintf (stderr, "Please reboot after running!\n");
		}
		else
		{
			close_device ();
			return 1;
		}
	}
	
	return 0;
}

void
close_device (void)
{
	(void) XHLock (major, minor, 0, key);
	(void) Dlock (0, drv);
}

int
set_size (long nblocks)
{
	nblocks <<= blocksize;
	
	if (nblocks > blocks)
		return 1;
	
	return 0;

}

# endif
