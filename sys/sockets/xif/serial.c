/*
 *	This file implements some sort of line discipline stuff using
 *	select and a separate process.
 *
 *	06/12/94, Kay Roemer.
 */

# include "global.h"
# include "netinfo.h"

# include "buf.h"

# include "serial.h"

# include "mint/basepage.h"
# include "mint/ioctl.h"

# include <mintbind.h>


# define STKSIZE		8192L
# define NSLBUFS		4

# define FDZERO(fd)		((fd) = 0)
# define FDSET(fd, set)		((set) |= 1L << (fd))
# define FDCLR(fd, set)		((set) &= ~(1L << (fd)))
# define FDISSET(fd, set)	((set) & (1L << (fd)))

static short glfd;
static char *pname = "u:\\pipe\\sld";
static struct slbuf *currsl, allslbufs[NSLBUFS];

static void
_send (void)
{
	(*currsl->send)(currsl);
}

static void
_recv (void)
{
	(*currsl->recv)(currsl);
}

/*
 * when calling Fcntl(fd, arg, cmd) directly gcc 2.5.8 does not detect that
 * *arg might be clobbered. Same for Supexec(). That is why we use those
 * wrapper functions instead ...
 */

static void
_sld (void)
{
	struct slcmd cmd;
	register struct slbuf *sl;
	short i, pipefd, p, tmout = 0;
	long r, rset, wset, wready, rready, space;
	
	pipefd = Fopen (pname, O_RDONLY|O_NDELAY);
	if (pipefd < 0)
	{
		char buf[128];
		
		ksprintf (buf, "sld: PANIC: Cannot open pipe (-> %i)\r\n", pipefd);
		Cconws (buf);
		
		return;
	}
	
	FDZERO (rset);
	FDZERO (wset);
	FDSET (pipefd, rset);
	
	for (;;)
	{
		wready = wset;
		rready = rset;
		/*
		 * Poll if some channels are active
		 */
		tmout = (rset & ~(1L << pipefd)) ? SL_VTIME : 0;
		r = Fselect (tmout, &rready, &wready, 0);
		if (r < 0)
			continue;
		
		if (FDISSET (pipefd, rready))
		{
			r = Fread (pipefd, 1, &cmd);
			if (r == 1 && cmd.slnum < NSLBUFS)
			{
				sl = &allslbufs[cmd.slnum];
				if (sl->flags & SL_INUSE) switch (cmd.cmd) {
				case SLCMD_OPEN:
				{
					char buf[128];
					
					ksprintf (buf, "sld: SLCMD_OPEN (sl->fd = %i)", sl->fd);
					Cconws (buf);
					
					r = sl->fd;
					sl->fd = Fdup (r);
					Fclose (r);
					if (sl->fd < 0)
					{
						Cconws ("sld: PANIC: Fdup failed\r\n");
						return;
					}
					FDSET (sl->fd, rset);
					break;
				}
				case SLCMD_CLOSE:
				{
					char buf[128];
					
					ksprintf (buf, "sld: SLCMD_CLOSE (sl->fd = %i)", sl->fd);
					Cconws (buf);
					
					FDCLR (sl->fd, rset);
					FDCLR (sl->fd, wset);
					FDCLR (sl->fd, wready);
					FDCLR (sl->fd, rready);
					Fclose (sl->fd);
					sl->flags &= ~(SL_INUSE|SL_SENDING|SL_CLOSING);
					break;
				}
				case SLCMD_SEND:
				{
					FDSET (sl->fd, wset);
					FDSET (sl->fd, wready); /* pretend we can write */
					currsl = sl;
					// Supexec (_send);
					_send ();
					break;
				}
				}
			}
		}
		
		for (i = 0; i < NSLBUFS; ++i)
		{
			long nr;
			
			sl = &allslbufs[i];
			if ((sl->flags & (SL_INUSE|SL_CLOSING)) != SL_INUSE)
				continue;
			
			/*
			 * We try to read something if
			 * 1) >= SL_VMIN chars arrived, or
			 * 2) no more input arrived in the last SL_VTIME msecs.
			 */
			nr = 0;
			Fcntl (sl->fd, &nr, FIONREAD);
			if (FDISSET (sl->fd, rready) || (nr && nr <= sl->nread))
			{
				p = sl->ihead;
				r = 1;
				if (p >= sl->itail)
				{
					space = sl->isize - p;
					if (sl->itail == 0) --space;
					if (space > 0)
					{
						r = Fread (sl->fd, space, &sl->ibuf[p]);
						if (r > 0)
						{
							p = (p + (short)r) & (sl->isize - 1);
							nr -= r;
						}
					}
				}
				if (r > 0 && p+1 < sl->itail)
				{
					space = sl->itail - p - 1;
					if (space > 0)
					{
						r = Fread (sl->fd, space, &sl->ibuf[p]);
						if (r > 0)
						{
							p = (p + (short)r) & (sl->isize - 1);
							nr -= r;
						}
					}
				}
				if (r < 0)
					Cconws ("sld: Fread failed\r\n");
				sl->ihead = p;
			}
			sl->nread = MAX (0, nr);
			if (sl->ihead != sl->itail)
			{
				currsl = sl;
				// Supexec (_recv);
				_recv ();
			}
			
			if (!(sl->flags & SL_SENDING))
				continue;
			
			/*
			 * We send something if
			 * 1) connection was idle some time, or
			 * 2) space did not increase for SL_VTIME, or
			 * 3) can send whole buf, or
			 * 4) can send at least SL_VMIN
			 */
			nr = 0;
			Fcntl (sl->fd, &nr, FIONWRITE);
			if (FDISSET (sl->fd, wready) || (nr && nr <= sl->nwrite) ||
			nr >= SL_VMIN || nr >= SL_OUSED (sl))
			{
				FDSET (sl->fd, wset);
				p = sl->otail;
				r = 1;
				if (p > sl->ohead)
				{
					space = sl->osize - p;
					if (space > 0)
					{
						r = Fwrite (sl->fd, space, &sl->obuf[p]);
						if (r > 0)
						{
			    				p = (p + (short)r) & (sl->osize - 1);
			    				nr -= r;
						}
					}
				}
				if (r > 0 && p < sl->ohead)
				{
					space = sl->ohead - p;
					if (space > 0)
					{
						r = Fwrite (sl->fd, space, &sl->obuf[p]);
						if (r > 0)
						{
							p = (p + (short)r) & (sl->osize - 1);
							nr -= r;
						}
					}
				}
				if (r < 0)
					Cconws ("sld: Fwrite failed\r\n");
				sl->otail = p;
				/*
				 * Produce more output if at least a quarter of
				 * our output buffer is free
				 */
				if (4*SL_OFREE (sl) >= sl->osize)
				{
					if (sl->ohead == sl->otail)
					{
						sl->flags &= ~SL_SENDING;
						FDCLR (sl->fd, wset);
					}
					currsl = sl;
					// Supexec (_send);
					_send ();
					if (sl->ohead != sl->otail)
					{
						sl->flags |= SL_SENDING;
						FDSET (sl->fd, wset);
					}
				}
			}
			sl->nwrite = MAX (0, nr);
			/*
			 * If buffer is tiny then turn off this descriptor
			 * and wait up to SL_VMIN until next retry.
			 * The descriptor is turned on again after the
			 * next Fselect().
			 */
			if (sl->nwrite < SL_OUSED (sl) && sl->nwrite < SL_VMIN)
				FDCLR (sl->fd, wset);
		}
	}
}

static int
sld (BASEPAGE *bp)
{
	// setstack ((char *) bp + STKSIZE);
	
	// bp->p_tbase = bp->p_parent->p_tbase;
	// bp->p_tlen  = bp->p_parent->p_tlen;
	// bp->p_dbase = bp->p_parent->p_dbase;
	// bp->p_dlen  = bp->p_parent->p_dlen;
	// bp->p_bbase = bp->p_parent->p_bbase;
	// bp->p_blen  = bp->p_parent->p_blen;
	
	(void) Pdomain (1);
	(void) Pnice (-5);
	(void) Psigblock (-1L);
	_sld ();
	(void) Pterm (0);
	
	return 0;
}

long
serial_init (void)
{
	BASEPAGE *b;
	
	glfd = f_open (pname, O_NDELAY|O_WRONLY|O_CREAT|O_GLOBAL);
	if (glfd < 100)
		glfd += 100;
	
	f_chmod (pname, 0600);
	
	b = (BASEPAGE *) p_exec (PE_CBASEPAGE, 0L, "", 0L);
	m_shrink (0, (long) b, STKSIZE + 256L);
	
	b->p_tbase = (long) sld;
	b->p_hitpa = (long) b + STKSIZE + 256L;
	
	return p_exec (104, "sld", b, 0L);
}

long
serial_make_raw (short fd)
{
	short vmin[2];
	struct sgttyb sg;
	
	if (f_cntl (fd, (long) &sg, TIOCGETP) < 0)
		return -1;
	
	sg.sg_flags &= ~(T_CRMOD|T_CBREAK|T_ECHO|T_XKEY|T_TOSTOP|T_EVENP|T_ODDP);
	sg.sg_flags |= T_RAW;
	if (f_cntl (fd, (long) &sg, TIOCSETP) < 0)
		return -1;
	
	/*
	 * VTIME we (must) do ourselves...
	 */
	vmin[0] = SL_VMIN;
	vmin[1] = 0;
	if (f_cntl (fd, (long) vmin, TIOCSVMIN) < 0)
		return -1;
	
	return 0;
}

struct slbuf *
serial_open (struct netif *nif, char *device,
		short (*send) (struct slbuf *),
		short (*recv) (struct slbuf *))
{
	struct slbuf *sl;
	struct slcmd cmd;
	short i;
	long r;
	
	sl = 0;
	for (i = 0; i < NSLBUFS; ++i)
	{
		if (!(allslbufs[i].flags & SL_INUSE))
		{
			sl = &allslbufs[i];
			if (sl->ibuf) kfree (sl->ibuf);
			if (sl->obuf) kfree (sl->obuf);
			sl->ibuf = sl->obuf = 0;
		}
	}
	if (sl == 0)
	{
		DEBUG (("serial_open: out of sl bufs"));
		return 0;
	}
	
	sl->flags = SL_INUSE;
	sl->nif   = nif;
	sl->send  = send;
	sl->recv  = recv;
	sl->nread = 0;
	sl->nwrite= 0;
	sl->isize = SL_IBUFSIZE;
	sl->osize = SL_OBUFSIZE;
	sl->ihead = sl->itail = 0;
	sl->ohead = sl->otail = 0;
	sl->ibuf  = kmalloc (SL_IBUFSIZE);
	sl->obuf  = kmalloc (SL_OBUFSIZE);
	
	if (!sl->ibuf || !sl->obuf)
	{
		DEBUG (("serial_open: no mem for buffers"));
		if (sl->ibuf) kfree (sl->ibuf);
		if (sl->obuf) kfree (sl->obuf);
		sl->ibuf = sl->obuf = 0;
		sl->flags &= ~SL_INUSE;
		return 0;
	}
	
	strncpy (sl->dev, device, sizeof (sl->dev));
	sl->dev[sizeof (sl->dev) - 1] = '\0';
	
	sl->fd = f_open (sl->dev, O_RDWR|O_NDELAY|O_GLOBAL);
	if (sl->fd < 0)
	{
		DEBUG (("serial_open: fopen(%s) returned %d", sl->fd));
		kfree (sl->ibuf);
		kfree (sl->obuf);
		sl->ibuf = sl->obuf = 0;
		sl->flags &= ~SL_INUSE;
		return 0;
	}
	if (sl->fd < 100)
		sl->fd += 100;
	
	if (serial_make_raw (sl->fd))
	{
		DEBUG (("serial_open: cannot make tty raw."));
		f_close (sl->fd);
		kfree (sl->ibuf);
		kfree (sl->obuf);
		sl->ibuf = sl->obuf = 0;
		sl->flags &= ~SL_INUSE;
		return 0;
	}
	
	cmd.slnum = sl - allslbufs;
	cmd.cmd   = SLCMD_OPEN;
	r = f_write (glfd, 1, (char *) &cmd);
	if (r != 1)
	{
		DEBUG (("serial_open: fwrite() returned %ld", r));
		f_close (sl->fd);
		kfree (sl->ibuf);
		kfree (sl->obuf);
		sl->ibuf = sl->obuf = 0;
		sl->flags &= ~SL_INUSE;
		return 0;
	}
	
	return sl;
}

long
serial_close (struct slbuf *sl)
{
	struct slcmd cmd;
	long r;
	
	cmd.slnum = sl - allslbufs;
	cmd.cmd   = SLCMD_CLOSE;
	r = f_write (glfd, 1, (char *) &cmd);
	if (r != 1)
	{
		DEBUG (("serial_close: fwrite() returned %ld", r));
		return r ? r : -1;
	}
	
	sl->flags |= SL_CLOSING;
	return 0;
}

long
serial_send (struct slbuf *sl)
{
	struct slcmd cmd;
	long r;
	
	if (sl->flags & SL_SENDING)
		return 0;
	
	cmd.slnum = sl - allslbufs;
	cmd.cmd   = SLCMD_SEND;
	r = f_write (glfd, 1, (char *) &cmd);
	if (r != 1)
	{
		DEBUG (("serial_send: fwrite() returned %ld", r));
		return r ? r : -1;
	}
	
	sl->flags |= SL_SENDING;
	return 0;
}
