/*
 *	BSD compatible packet filter.
 *
 *	10/22/95, Kay Roemer.
 */

# include "bpf.h"

# include "in.h"
# include "ifeth.h"
# include "ip.h"

# include "buf.h"
# include "timer.h"
# include "util.h"

# include <mint/asm.h>
# include <mint/dcntl.h>
# include <mint/file.h>


# define BPF_MAXPACKETS	60
# define BPF_READBUF	(8*1024)
# define BPF_RESERVE	100
# define BPF_HDRLEN	(BPF_WORDALIGN (SIZEOF_BPF_HDR))

struct bpf
{
	struct bpf	*link;		/* next desc in global list */
	struct bpf	*next;		/* next desc for this if */
	struct ifq	recvq;		/* packet input queue */
	struct netif	*nif;		/* the if we are listening to */
	struct bpf_insn *prog;		/* filter program */
	short		proglen;	/* # of insns in the filter program */
	long		tmout;		/* read timeout */
	struct event	evt;		/* timeout event */
	long		rsel;		/* read-selecting process */
	volatile short	flags;		/* what do you think? */
# define BPF_OPEN 0x0001
# define BPF_WAKE 0x0002
	long		hdrlen;		/* MAC header length */
	long		hwtype;		/* hardware type */

	long		in_pkts;	/* # of packets received */
	long		in_drop;	/* # of packets dropped */

	long		s_time;		/* GMT at open() */
	long		s_ticks;	/* HZ200 ticks at open() */
};

static long	bpf_open	(FILEPTR *);
static long	bpf_write	(FILEPTR *, const char *, long);
static long	bpf_read	(FILEPTR *, char *, long);
static long	bpf_lseek	(FILEPTR *, long, int);
static long	bpf_ioctl	(FILEPTR *, int, void *);
static long	bpf_datime	(FILEPTR *, ushort *, int);
static long	bpf_close	(FILEPTR *, int);
static long	bpf_select	(FILEPTR *, long, int);
static void	bpf_unselect	(FILEPTR *, long, int);

static struct bpf *bpf_create	(void);
static void	bpf_release	(struct bpf *);
static long	bpf_attach	(struct bpf *, struct ifreq *);
static void	bpf_reset	(struct bpf *);
static long	bpf_sfilter	(struct bpf *, struct bpf_program *);
static void	bpf_handler	(long);

static struct devdrv bpf_dev =
{
	open:		bpf_open,
	write:		bpf_write,
	read:		bpf_read,
	lseek:		bpf_lseek,
	ioctl:		bpf_ioctl,
	datime:		bpf_datime,
	close:		bpf_close,
	select:		bpf_select,
	unselect:	bpf_unselect
};

static struct dev_descr	bpf_desc =
{
	driver:		&bpf_dev,
	fmode:		S_IFCHR | S_IRUSR | S_IWUSR
};

/*
 * BPF initialization
 */
void
bpf_init (void)
{
	long r;
	
	r = d_cntl (DEV_INSTALL, "u:\\dev\\bpf0", (long) &bpf_desc);
	if (!r || r == ENOSYS)
		c_conws ("Cannot install BPF device\r\n");
}

static struct bpf *allbpfs = 0;

/*
 * create a new packet filter
 */
static struct bpf *
bpf_create (void)
{
	struct bpf *bpf;
	
	bpf = kmalloc (sizeof (*bpf));
	if (!bpf)
		return NULL;
	
	bzero (bpf, sizeof (*bpf));
	
	bpf->recvq.maxqlen = BPF_MAXPACKETS;
	bpf->link = allbpfs;
	allbpfs = bpf;
	
	return bpf;
}

/*
 * destroy a packet filter
 */
static void
bpf_release (struct bpf *bpf)
{
	struct bpf *bpf2, **prev;
	ushort sr;
	
	sr = spl7 ();
	if_flushq (&bpf->recvq);
	if (bpf->flags & BPF_OPEN)
	{
		prev = &bpf->nif->bpf;
		bpf2 = *prev;
		for ( ; bpf2; prev = &bpf2->next, bpf2 = *prev)
		{
			if (bpf2 == bpf)
			{
				/*
				 * found
				 */
				*prev = bpf->next;
				break;
			}
		}
		if (!bpf2)
		{
			spl (sr);
			FATAL ("bpf_release: bpf not in list!");
		}
	}
	spl (sr);
	
	prev = &allbpfs;
	for (bpf2 = *prev; bpf2; prev = &bpf->link, bpf2 = *prev)
	{
		if (bpf2 == bpf)
		{
			/*
			 * found
			 */
			*prev = bpf->link;
			break;
		}
	}
	
	if (!bpf2)
		FATAL ("bpf_release: bpf not in global list!");
	
	if (bpf->prog)
		kfree (bpf->prog);
	
	kfree (bpf);
}

/*
 * attach an interface to the packet filter
 */
static long
bpf_attach (struct bpf *bpf, struct ifreq *ifr)
{
	ushort sr;
	
	sr = spl7 ();
	if (bpf->flags & BPF_OPEN)
	{
		struct bpf *bpf2, **prev;
		
		prev = &bpf->nif->bpf;
		bpf2 = *prev;
		for ( ; bpf2; prev = &bpf2->next, bpf2 = *prev)
		{
			if (bpf2 == bpf)
			{
				/*
				 * found
				 */
				*prev = bpf->next;
				break;
			}
		}
		bpf->next = 0;
		bpf->flags &= ~BPF_OPEN;
		
		if (!bpf2)
		{
			spl (sr);
			FATAL ("bpf_attach: bpf not in list!");
		}
	}
	spl (sr);
	
	bpf->nif = if_name2if (ifr->ifr_name);
	if (!bpf->nif)
		return ENOENT;
	
	if (!(bpf->nif->flags & IFF_UP))
		return ENETDOWN;
	
	bpf->next = bpf->nif->bpf;
	bpf->nif->bpf = bpf;
	
	switch (bpf->nif->hwtype)
	{
		case HWTYPE_NONE:
			bpf->hwtype = DLT_NULL;
			bpf->hdrlen = 0;
			break;
			
		case HWTYPE_SLIP:
		case HWTYPE_PLIP:
			bpf->hwtype = DLT_SLIP;
			bpf->hdrlen = 0;
			break;
			
		case HWTYPE_PPP:
			bpf->hwtype = DLT_PPP;
			bpf->hdrlen = 4;
			break;
			
		case HWTYPE_ETH:
			bpf->hwtype = DLT_EN10MB;
			bpf->hdrlen = sizeof (struct eth_dgram);
			break;
			
		default:
			FATAL ("bpf_attach: unknown hardware type for if %s", bpf->nif->name);
	}
	
	bpf->flags |= BPF_OPEN;
	return 0;
}

/*
 * flush queue and reset counters
 */
static void
bpf_reset (struct bpf *bpf)
{
	if_flushq (&bpf->recvq);
	bpf->in_pkts = 0;
	bpf->in_drop = 0;
}

/*
 * set new filer program
 */
static long
bpf_sfilter (struct bpf *bpf, struct bpf_program *prog)
{
	struct bpf_insn *oprog, *nprog;
	long size;
	ushort sr;
	
	oprog = bpf->prog;
	if (prog->bf_insns == 0)
	{
		if (prog->bf_len != 0)
			return EINVAL;
		sr = spl7 ();
		bpf->prog = 0;
		bpf->proglen = 0;
		bpf_reset (bpf);
		spl (sr);
		if (oprog)
			kfree (oprog);
		return 0;
	}
	
	if (prog->bf_len > BPF_MAXINSNS)
		return EINVAL;
	
	size = prog->bf_len * sizeof (struct bpf_insn);
	nprog = kmalloc (size);
	if (!nprog)
		return ENOMEM;
	memcpy (nprog, prog->bf_insns, size);
	
	if (bpf_validate (nprog, prog->bf_len))
	{
		sr = spl7 ();
		bpf->prog = nprog;
		bpf->proglen = prog->bf_len;
		bpf_reset (bpf);
		spl (sr);
		if (oprog)
			kfree (oprog);
		return 0;
	}
	
	kfree (nprog);
	return EINVAL;
}


static volatile short have_tmout;

/*
 * top half input handler.
 */
static void
bpf_handler (long proc)
{
	struct bpf *bpf;
	
	have_tmout = 0;
	for (bpf = allbpfs; bpf; bpf = bpf->link)
	{
		ushort sr = spl7 ();
		if (bpf->flags & BPF_WAKE)
		{
			bpf->flags &= ~BPF_WAKE;
			spl (sr);
			wake (IO_Q, (long) bpf);
			if (bpf->rsel)
				wakeselect (bpf->rsel);
		}
		else
			spl (sr);
	}
}

/*
 * bottom half input handler, called from interface input routine.
 * packet contains MAC header.
 */
long
bpf_input (struct netif *nif, BUF *buf)
{
	struct bpf *bpf;
	long caplen, pktlen, snaplen, ticks, align;
	struct bpf_hdr *hp;
	BUF *buf2;
	ushort sr;
	
	for (bpf = nif->bpf; bpf; bpf = bpf->next)
	{
		if (!(bpf->flags & BPF_OPEN))
			continue;
		bpf->in_pkts++;
		pktlen = buf->dend - buf->dstart;
		snaplen = bpf_filter (bpf->prog, buf->dstart, pktlen, pktlen);
		if (snaplen == 0)
			continue;
		
		caplen = BPF_HDRLEN + MIN (snaplen, pktlen);
		if (caplen > BPF_READBUF-BPF_ALIGNMENT)
			caplen = BPF_READBUF-BPF_ALIGNMENT;
		
		/*
		 * We have to be careful. A driver with higher interrupt
		 * priority may grap the buffer and manipulate the dend
		 * and dstart pointer while we are on them.
		 */
		sr = spl7 ();
		if ((buf2 = bpf->recvq.qlast[IF_PRIORITIES-1]) &&
		    BUF_TRAIL_SPACE (buf2) >=
		      caplen+BPF_ALIGNMENT+BPF_RESERVE/2)
		{
			/*
			 * We can prepend the packet to a previously
			 * allocated buf. We leave BPF_RESERVE/2 bytes
			 * of space at the end of the buf.
			 */
			align = BPF_WORDALIGN ((long)buf2->dend -
				(long)buf2->dstart);
			buf2->dend = buf2->dstart + align;
		}
		else
		{
			if (bpf->recvq.qlen >= BPF_MAXPACKETS)
			{
				spl (sr);
				++bpf->in_drop;
				continue;
			}
			spl (sr);
			buf2 = buf_alloc (caplen+BPF_RESERVE,BPF_RESERVE/2,
				BUF_ATOMIC);
			if (!buf2)
			{
				++bpf->in_drop;
				continue;
			}
			sr = spl7 ();
			if (if_enqueue (&bpf->recvq, buf2, IF_PRIORITIES-1))
			{
				spl (sr);
				++bpf->in_drop;
				continue;
			}
		}
		hp = (struct bpf_hdr *)buf2->dend;
		buf2->dend += caplen;
		/*
		 * All manipulations on buf pointers done. So we can switch
		 * interrupts back on.
		 */
		bpf->flags |= BPF_WAKE;
		if (!have_tmout)
		{
			have_tmout = 1;
			spl (sr);
			addroottimeout (0, bpf_handler, 1);
		}
		else
			spl (sr);
		
		ticks = (GETTIME () - bpf->s_ticks) * 5;
		hp->bh_tstamp.tv_sec  = bpf->s_time + ticks/1000;
		hp->bh_tstamp.tv_usec = (ticks%1000)*1000;
		
		hp->bh_datalen = pktlen;
		hp->bh_hdrlen = BPF_HDRLEN;
		hp->bh_caplen = (caplen -= BPF_HDRLEN);
		
		memcpy ((char *) hp + BPF_HDRLEN, buf->dstart, caplen);
	}
	
	return 0;
}

/*
 * /dev/bpf device driver
 */
static long
bpf_open (FILEPTR *fp)
{
	struct bpf *bpf;
	
	if (p_geteuid ())
		return EACCES;
	
	bpf = bpf_create ();
	if (!bpf)
		return ENOMEM;
	
	bpf->s_time = unixtime (t_gettime (), t_getdate ());
	bpf->s_ticks = GETTIME ();
	
	fp->devinfo = (long) bpf;
	return 0;
}

/*
 * Passed data contains the MAC header!
 */
static long
bpf_write (FILEPTR *fp, const char *buf, long nbytes)
{
	struct bpf *bpf = (struct bpf *) fp->devinfo;
	short daddrlen, pktype;
	void *daddr;
	BUF *b;
	
	if (!(bpf->flags & BPF_OPEN))
		/*
		 * this maps to ENXIO in the lib
		 */
		return ENXIO;
	
	b = buf_alloc (nbytes+BPF_RESERVE, BPF_RESERVE/2, BUF_NORMAL);
	if (!b)
		return ENOMEM;
	
	switch (bpf->nif->hwtype)
	{
		case HWTYPE_NONE:
		case HWTYPE_SLIP:
		case HWTYPE_PLIP:
			/*
			 * Assume IP... and pass IP dst as next hop.
			 */
			daddr = &((struct ip_dgram *)buf)->daddr;
			daddrlen = 4;
			pktype = PKTYPE_IP;
			break;
			
		case HWTYPE_PPP:
			/*
			 * may not be IP...
			 */
			daddr = &((struct ip_dgram *)(buf + bpf->hdrlen))->daddr;
			daddrlen = 4;
			pktype = PKTYPE_IP;
			break;
			
		case HWTYPE_ETH:
			daddr = &((struct eth_dgram *)buf)->daddr;
			daddrlen = ETH_ALEN;
			memcpy (&pktype, &((struct eth_dgram *)buf)->proto, 2);
			if (pktype >= 1536)
				pktype = ETHPROTO_8023;
			break;
			
		default:
			FATAL ("bpf_write: unknown hardware type for if %s", bpf->nif->name);
			return EINTERNAL;
	}
	
	if (nbytes < bpf->hdrlen)
		return EINVAL;
	
	memcpy (b->dstart, buf + bpf->hdrlen, nbytes - bpf->hdrlen);
	b->dend += nbytes - bpf->hdrlen;
	
	return (*bpf->nif->output) (bpf->nif, b, daddr, daddrlen, pktype);
}

static void
wakemeup (long arg)
{
	wake (IO_Q, arg);
}

/*
 * the following data ist returned by read():
 *
 * N					: struct bpf_hdr  #1
 * N + bh_hdrlen			: packet snapshot #1
 * N + bh_hdrlen + ALIGN4(bh_caplen)	: struct bpf_hdr  #2
 * ...
 */
static long
bpf_read (FILEPTR *fp, char *buf, long nbytes)
{
	struct bpf *bpf = (struct bpf *) fp->devinfo;
	BUF *b;
	
	if (!(bpf->flags & BPF_OPEN))
		return ENXIO;
	
	if (nbytes < BPF_READBUF)
		return EINVAL;
	
	if (!(b = if_dequeue (&bpf->recvq)))
	{
		if (fp->flags & O_NDELAY)
			return EWOULDBLOCK;
		if (bpf->tmout)
			event_add (&bpf->evt, bpf->tmout, wakemeup, (long)bpf);
		if (isleep (IO_Q, (long)bpf))
			return EINTR;
		if (bpf->tmout)
			event_del (&bpf->evt);
		if (!(b = if_dequeue (&bpf->recvq)))
			return EWOULDBLOCK;
	}
	
	nbytes = b->dend - b->dstart;
	memcpy (buf, b->dstart, nbytes);
	buf_deref (b, BUF_NORMAL);
	
	return nbytes;
}

static long
bpf_lseek (FILEPTR *fp, long where, int whence)
{
	return EACCES;
}

static long
bpf_ioctl (FILEPTR *fp, int cmd, void *arg)
{
	struct bpf *bpf = (struct bpf *) fp->devinfo;
	BUF *b;
	ushort sr;
	
	switch (cmd)
	{
		case FIONREAD:
			b = bpf->recvq.qfirst[IF_PRIORITIES-1];
			*(long *)arg = b ? b->dend - b->dstart : 0;
			return 0;
			
		case FIONWRITE:
			*(long *)arg = 1;
			return 0;
			
		case FIOEXCEPT:
			*(long *)arg = 0;
			return 0;
			
		case SIOCGIFADDR:
			return if_ioctl (SIOCGIFADDR, (long)arg);
			
		case BIOCGFLEN:
			/*
			 * max # of insns in a filter program
			 */
			*(long *)arg = BPF_MAXINSNS;
			return 0;
			
		case BIOCGBLEN:
			/*
			 * read() buffer length
			 */
			*(long *)arg = BPF_READBUF;
			return 0;
			
		case BIOCSETF:
			/*
			 * set filter program
			 */
			return bpf_sfilter (bpf, (struct bpf_program *)arg);
			
		case BIOCFLUSH:
			/*
			 * flush in-queue
			 */
			sr = spl7 ();
			bpf_reset (bpf);
			spl (sr);
			return 0;
			
		case BIOCPROMISC:
			/*
			 * set if to promisc mode (not yet)
			 */
			if (!(bpf->flags & BPF_OPEN))
				return ENXIO;
			return 0;
			
		case BIOCGDLT:
			/*
			 * Get link level hardware type
			 */
			if (!(bpf->flags & BPF_OPEN))
				return ENXIO;
			*(long *) arg = bpf->hwtype;
			return 0;
			
		case BIOCGETIF:
			/*
			 * get interface name
			 */
			if (bpf->flags & BPF_OPEN)
			{
				struct ifreq *ifr = (struct ifreq *) arg;
				ksprintf (ifr->ifr_name, "%s%d", bpf->nif->name, bpf->nif->unit);
				return 0;
			}
			return ENXIO;
			
		case BIOCSETIF:
			/*
			 * attach to interface
			 */
			return bpf_attach (bpf, (struct ifreq *)arg);
			
		case BIOCSRTIMEOUT:
			/*
			 * set read timeout
			 */
			bpf->tmout = *(long *)arg / EVTGRAN;
			return 0;
			
		case BIOCGRTIMEOUT:
			/*
			 * get read timeout
			 */
			*(long *)arg = bpf->tmout * EVTGRAN;
			
		case BIOCGSTATS:
			((struct bpf_stat *)arg)->bs_recv = bpf->in_pkts;
			((struct bpf_stat *)arg)->bs_drop = bpf->in_drop;
			return 0;
			
		case BIOCIMMEDIATE:
			/*
			 * set immediate mode
			 */
			return 0;
			
		case BIOCVERSION:
			/*
			 * get verion info
			 */
			((struct bpf_version *)arg)->bv_major = BPF_MAJOR_VERSION;
			((struct bpf_version *)arg)->bv_minor = BPF_MINOR_VERSION;
			return 0;
	}
	
	return ENOSYS;
}

static long
bpf_datime (FILEPTR *fp, ushort *timeptr, int mode)
{
	if (mode == 0)
	{
		timeptr[0] = t_gettime ();
		timeptr[1] = t_getdate ();
		
		return 0;
	}
	
	return EACCES;
}

static long
bpf_close (FILEPTR *fp, int pid)
{
	struct bpf *bpf = (struct bpf *) fp->devinfo;
	
	if (bpf->rsel)
		wakeselect (bpf->rsel);
	
	wake (IO_Q, (long) bpf);
	
	if (fp->links <= 0)
		bpf_release (bpf);
	
	return 0;
}

static long
bpf_select (FILEPTR *fp, long proc, int mode)
{
	struct bpf *bpf = (struct bpf *)fp->devinfo;
	
	switch (mode)
	{
		case O_RDONLY:
			if (bpf->recvq.qfirst[IF_PRIORITIES-1])
				return 1;
			if (bpf->rsel == 0)
			{
				bpf->rsel = proc;
				return 0;
			}
			return 2;
			
		case O_WRONLY:
			return 1;
			
		case O_RDWR:
			return 0;
	}
	
	return 0;
}

static void
bpf_unselect (FILEPTR *fp, long proc, int mode)
{
	struct bpf *bpf = (struct bpf *) fp->devinfo;
	
	switch (mode)
	{
		case O_RDONLY:
			if (proc == bpf->rsel)
				bpf->rsel = 0;
			break;
			
		case O_WRONLY:
		case O_RDWR:
			break;
	}
}
