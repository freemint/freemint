/*
 * interface to sound hardware drivers for /dev/audio.
 *
 * 11/03/95, Kay Roemer.
 *
 * 9802, John Blakeley - supporting F030's recording ability.
 */

# ifndef _device_h
# define _device_h

/*
 * sample buffer - 2 * 32K
 */
# define BUFSIZE (0x8000L - sizeof (long))

/*
 * sound hardware descriptor
 */
struct device
{
	short	maxchans;		/* max. no. of channels supported */
	short	curchans;		/* 1 = mono, 2 = stereo, 4 = quad, etc */
	short	format_map;		/* bitmask of AFMT_???? */
	short	curformat;
	short	ssize;	/* 8 = 8bits, 16 = 16bits */
	short	status;			/* rec < 0 < playing */
	long	srate;
	void	(*reset) (void);
	void	*(*copyfn) (void *dst, const void *src, long len);
	long	(*copyin) (const char *buf, long len);
	long	(*copyout) (char *buf, long len);
	long	(*ioctl) (short mode, void *arg);
	long	(*wspace) (void);
	long	(*rspace) (void);
	long	(*mix_ioctl) (short cmd, void *arg);
};

extern struct device thedev;

struct dmabuf
{
	long used;
	unsigned char buf[BUFSIZE];
};

extern struct dmabuf *dmab;

/*
 * index into dmab. points to buffer currently being filled
 * by write().
 */
extern volatile short bufidx;

/*
 * write selector.
 */
extern long audio_rsel;

/*
 * asm stuff
 */
void  new_timera_vector (void);
void  new_gpi7_vector (void);
void  psg_player (void);
void  (*timer_func) (void);


# endif /* _device_h */
