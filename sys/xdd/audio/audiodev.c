/*
 *	/dev/audio for Atari Ste, MegaSte, TT, Falcon running Mint.
 *
 *	Based on audiodev 0.1 by Charles Briscoe-Smith, which is in
 *	turn based on lpdev 0.6 by Thierry Bousch.
 *
 *	10/28/95, Kay Roemer.
 *
 *	9705, John Blakeley - version 0.8 - now works on the Falcon, properly.
 *	9802, John Blakeley - version 0.9 - added support for 16bit F030 support.
 */

# include "global.h"
# include "mint/dcntl.h"
# include "mint/file.h"

# include "afmts.h"

#include "device.h"
#include "mfp.h"


#define AUDIO_VERSION	"0.91"

static long	audio_open	(FILEPTR *f);
static long	audio_write	(FILEPTR *f, const char *buf, long bytes);
static long	audio_read	(FILEPTR *f, char *buf, long bytes);
static long	audio_lseek	(FILEPTR *f, long where, int whence);
static long	audio_ioctl	(FILEPTR *f, int mode, void *buf);
static long	audio_datime	(FILEPTR *f, ushort *timeptr, int rwflag);
static long	audio_close	(FILEPTR *f, int pid);
static long	audio_select	(FILEPTR *f, long proc, int mode);
static void	audio_unselect	(FILEPTR *f, long proc, int mode);

static long	audio_install	(void);


DEVDRV audio_device =
{
	audio_open,
	audio_write,
	audio_read,
	audio_lseek,
	audio_ioctl,
	audio_datime,
	audio_close,
	audio_select,
	audio_unselect
};

static struct dev_descr audio_descr = { &audio_device };

long audio_rsel = 0L;
static struct flock audio_lock = { F_WRLCK, 0, 0L, 0L, -1 };

volatile short bufidx = 0;

struct device thedev;


extern long psg_init (struct device *);
extern long dma_init (struct device *);
extern long fal_init (struct device *);
extern long fal_mix_init (struct device *);
extern long lmc_init (struct device *);
extern long nomix_init (struct device *);

struct init_entry
{
	long	(*init) (struct device *);
	char	*name;
};

static struct init_entry players[] =
{
	{ fal_init, "F030-DMA" },
	{ dma_init, "STe-DMA" },
	{ psg_init, "PSG" },
	{ 0, 0 }
};

static struct init_entry mixers[] =
{
	{ fal_mix_init, "F030-MIXER" },
	{ lmc_init, "LMC" },
	{ nomix_init, "NO-MIXER" },
	{ 0, 0 }
};


ulong memory = 0;
struct kerinfo *kernel;
struct dmabuf *dmab;

DEVDRV *
init (struct kerinfo *k)
{
	char msg[128];
	long mch;
	long r;

	kernel = k;

	ksprintf (msg, "\r\nAudio device for Mint, version %s\r\n",
		AUDIO_VERSION);
	c_conws (msg);
	c_conws ("(w) 1995 Charles Briscoe-Smith\r\n");
	c_conws ("(w) 1995 Kay Roemer\r\n");
	c_conws ("(w) 1997, 1998 John Blakeley\r\n");
	c_conws ("(w) 2001 Frank Naumann\r\n");

# define SSYS_GETCOOKIE	8
# define COOKIE__MCH	0x5f4d4348L
/* values of MCH cookie
 */
# define ST		0
# define STE		0x00010000L
# define MEGASTE	0x00010010L
# define TT		0x00020000L
# define FALCON		0x00030000L
# define MILAN_C	0x00040000L
	if ((s_system (SSYS_GETCOOKIE, COOKIE__MCH, (long) &mch) == 0)
		&& (mch == MILAN_C))
	{
		c_conws ("\033pThis driver don't work on a Milan!\033q\r\n");
		return NULL;
	}
	
	if ((MINT_MAJOR == 0)
		|| ((MINT_MAJOR == 1) && ((MINT_MINOR < 15) || (MINT_KVERSION < 2))))
	{
		c_conws ("audio NOT installed, MiNT too old.\r\n\r\n");
		return NULL;
	}
	
	dmab = (struct dmabuf *) m_xalloc (sizeof (*dmab) * 2, 0x20|0);
	if (!dmab)
	{
		c_conws ("audio NOT installed, out of memory.\r\n\r\n");
		return NULL;
	}
	
	ksprintf (msg, "\r\n!!! dmab = %lx !!!\r\n", dmab);
	c_conws (msg);
	
	if (audio_install ())
	{
		c_conws ("audio NOT installed.\r\n\r\n");
		return NULL;
	}
	r = d_cntl (DEV_INSTALL, "u:\\dev\\audio", (long)&audio_descr);
	if (!r || r == ENOSYS)
	{
		c_conws ("Cannot install audio device\r\n");
		return NULL;
	}
	c_conws ("\r\n");
	return (DEVDRV *) 1;
}

/*
 * install things
 */
static long
audio_install (void)
{
	char msg[128];
	int i, j;

	dmab[0].used = dmab[1].used = 0;

	for (i = 0; players[i].init; ++i) {
		if (!(*players[i].init) (&thedev))
			break;
	}
	if (!players[i].init) {
		c_conws ("No sound hardware found\r\n");
		return 1;
	}
	for (j = 0; mixers[j].init; ++j) {
		if (!(*mixers[j].init) (&thedev))
			break;
	}
	if (!mixers[j].init) {
		c_conws ("No mixer hardware found\r\n");
		return 1;
	}

	thedev.copyfn = memcpy;

	ksprintf (msg, "hardware: %s, %s\r\n",
		players[i].name, mixers[i].name);
	c_conws (msg);
	return 0;
}

/*
 * /dev/audio device driver
 */

static long
audio_open (FILEPTR *fp)
{
	/* Nothing to do */
	return 0;
}

static long
audio_write (FILEPTR *fp, const char *buf, long nbytes)
{
	long r, bufsize;

	bufsize = nbytes;
	while (nbytes > 0) {
		while ((*thedev.wspace) () == 0) {
			if (fp->flags & O_NDELAY) {
				return bufsize - nbytes;
			}
			nap (100);
		}
		r = (*thedev.copyin) (buf, nbytes);
		if (r < 0) {
			if (nbytes == bufsize) {
				return r;
			}
			break;
		}
		nbytes -= r;
		buf += r;
	}
	return bufsize - nbytes;
}

static long
audio_read (FILEPTR *fp, char *buf, long nbytes)
{
	return EACCES;
}

static long
audio_lseek (FILEPTR *fp, long where, int whence)
{
	return 0;
}

static long
audio_ioctl (FILEPTR *fp, int mode, void *buf)
{
	struct flock *g;
	long arg = (long)buf;

	switch (mode) {
	case AIOCSVOLUME:
	case AIOCSLVOLUME:
	case AIOCSRVOLUME:
	case AIOCSBASS:
	case AIOCSTREBLE:
		return (*thedev.mix_ioctl) (mode, buf);

	case AIOCRESET:
		(*thedev.reset) ();
		return 0;

	case AIOCSYNC:
		while (thedev.status)
			nap (50);
		return 0;

	case AIOCGBLKSIZE:
		*(long *)arg = BUFSIZE;
		break;

	case AIOCGFMTS:
		*(long *)arg = (long) thedev.format_map;
		break;

	case AIOCSFMT:
		thedev.curformat = arg;
		switch (arg) {
		case AFMT_U8:
			thedev.copyfn = u8copy;
			thedev.ssize = 8;
			break;

		case AFMT_S8:
			thedev.copyfn = memcpy;
			thedev.ssize = 8;
			break;

		case AFMT_ULAW:
			thedev.copyfn = ulawcopy;
			thedev.ssize = 8;
			break;

		case AFMT_U16:
			if (thedev.curchans == 1)
				thedev.copyfn = mu16copy;
			else
				thedev.copyfn = u16copy;
			thedev.ssize = 16;
			break;

		case AFMT_S16:
			if (thedev.curchans == 1)
				thedev.copyfn = m16copy;
			else
				thedev.copyfn = memcpy;
			thedev.ssize = 16;
			break;

		default:
			return EINVAL;
		}
		break;

	case AIOCGSPEED:
	case AIOCSSPEED:
	case AIOCGCHAN:
	case AIOCSCHAN:
		return (*thedev.ioctl) (mode, buf);

	case FIONREAD:
		*(long *)buf = (*thedev.rspace) ();
		break;

	case FIONWRITE:
		*(long *)buf = (*thedev.wspace) ();
		break;

	case F_GETLK:
		g = (struct flock *)buf;
		if (audio_lock.l_pid >= 0)
			*g = audio_lock;
		else
			g->l_type = F_UNLCK;
		break;

	case F_SETLK:
	case F_SETLKW:
		g = (struct flock *)buf;
		switch (g->l_type) {
		case F_UNLCK:
			if (!(fp->flags & O_LOCK) ||
			    g->l_pid != audio_lock.l_pid)
				return ENSLOCK;
			else {
				fp->flags &= ~O_LOCK;
				audio_lock.l_pid = -1;
				wake (IO_Q, (long)&audio_lock);
			}
			break;

		case F_RDLCK:
			TRACE(("audio: read locks are ignored"));
			break;

		case F_WRLCK:
			while (audio_lock.l_pid >= 0) {
				if (mode == F_SETLK) {
					*g = audio_lock;
					return ELOCKED;
				}
				sleep (IO_Q, (long)&audio_lock);
			}
			fp->flags |= O_LOCK;
			audio_lock.l_pid = g->l_pid;
			break;

		default:
			DEBUG (("audio: invalid lock type"));
			return ENOSYS;
		}
		break;

	default:
		return ENOSYS;
	}
	return 0;
}

static long
audio_datime (FILEPTR *fp, ushort *timeptr, int rwflag)
{
	if (!rwflag) {
		timeptr[0] = t_gettime ();
		timeptr[1] = t_getdate ();
	}
	return 0;
}

static long
audio_close (FILEPTR *fp, int pid)
{
	if (audio_rsel)
		wakeselect (audio_rsel);
	if ((fp->flags & O_LOCK) && audio_lock.l_pid == pid) {
		fp->flags &= ~O_LOCK;
		audio_lock.l_pid = -1;
		wake (IO_Q, (long)&audio_lock);
	}
	return 0;
}

static long
audio_select (FILEPTR *fp, long proc, int mode)
{
	switch (mode) {
	case O_RDONLY:
		return 1;

	case O_WRONLY:
		if ((*thedev.wspace) () > 0) {
			return 1;
		}
		if (audio_rsel == 0) {
			audio_rsel = proc;
			return 0;
		}
		return 2;

	case O_RDWR:
		return 0;
	}
	return 0;
}

static void
audio_unselect (FILEPTR *fp, long proc,  int mode)
{
	if (mode == O_WRONLY && proc == audio_rsel)
		audio_rsel = 0;
}


