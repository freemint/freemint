
# ifndef _serial_h
# define _serial_h

# include "global.h"

# include "inet4/if.h"

# include <mint/config.h>
# include <mint/file.h>


# define SL_IUSED(s)	(((s)->ihead - (s)->itail) & ((s)->isize - 1))
# define SL_IFREE(s)	((s)->isize - 1 - SL_IUSED(s))
# define SL_OUSED(s)	(((s)->ohead - (s)->otail) & ((s)->osize - 1))
# define SL_OFREE(s)	((s)->osize - 1 - SL_OUSED(s))

/*
 * Output and input buffers sizes, must be powers of 2!
 */
# define SL_OBUFSIZE	512
# define SL_IBUFSIZE	1024

/*
 * VMIN and VTIME for tty input. VMIN is handled by the kernel, VTIME
 * we must do ourselfes, because we don't do blocking read()'s but use
 * Fselect().
 */
# define SL_VMIN		(SL_IBUFSIZE < 200 ? SL_IBUFSIZE/2 : 100)
# define SL_VTIME	40

struct slbuf
{
	volatile char	flags;
# define SL_INUSE	0x01		/* slbuf in use */
# define SL_SENDING	0x02		/* send in progress */
# define SL_CLOSING	0x04		/* close in progress */
	
	char		dev[PATH_MAX];	/* device name */
	short		fd;		/* file descriptor */
	struct netif	*nif;		/* interface this belongs to */
	
	short		isize;		/* input ring buffer size */
	char		*ibuf;		/* pointer to input buf */
	short		ihead;		/* input buffer head */
	short		itail;		/* output buffer tail */
	
	short		osize;		/* ditto for output */
	char		*obuf;		/* ditto for output */
	short		ohead;		/* ditto for output */
	short		otail;		/* ditto for output */
	
	short		(*send)(struct slbuf *);	/* send more */
	short		(*recv)(struct slbuf *);	/* recv more */
	
	long		nread;		/* bytes avail for reading */
	long		nwrite;		/* bytes avail for writing */
};

struct slcmd
{
	uchar		slnum:4;
	uchar		cmd:4;
# define SLCMD_OPEN	0
# define SLCMD_CLOSE	1
# define SLCMD_SEND	2
};


long		serial_init (void);

long		serial_close (struct slbuf *);
long		serial_send (struct slbuf *);
struct slbuf *	serial_open (struct netif *, char *,
				short (*) (struct slbuf *),
				short (*) (struct slbuf *));


# endif /* _serial_h */
