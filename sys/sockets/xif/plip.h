/*
 *	Defines and structures for the parallel line IP driver.
 *
 *	94/12/03, Kay Roemer.
 */

# ifndef _plip_h
# define _plip_h


# define ESC		219
# define END		192
# define ESC_ESC	221
# define ESC_END	220

# define PLIP_CHANNELS	1
# define PLIP_MTU	2000
# define PLIP_RETRIES	3
# define PLIP_TIMEOUT	1
# define PLIP_MAXTMOUT	10
# define PLIP_VERSION	"PLIP v0.7, one channel\n\r"

# define HZ200		(*(volatile long *)0x4baL)

/*
 * Wait 'ticks' * 5 ms.
 */
# define WAIT(ticks) { \
	long stamp = HZ200; \
	while (HZ200 - stamp <= (ticks)); \
}

/*
 * Retry 'expr' for 'ticks' * 5 ms or until 'expr' gets true. Returns
 * false if time exceeded and true otherwise (ie 'expr' gets true).
 */
# define RETRY(ticks, expr) ({ \
	long n, stamp = HZ200; \
	while ((n = HZ200) - stamp <= (ticks) && !(expr)); \
	n - stamp <= (ticks); \
})

/*
 * XXX Rather ugly hack.
 */
# define HARD_WAIT()	__asm__ volatile(\
"nop;nop;nop;nop;nop;nop;nop;nop;nop;nop\n"\
"nop;nop;nop;nop;nop;nop;nop;nop;nop;nop\n"\
"nop;nop;nop;nop;nop;nop;nop;nop;nop;nop\n"\
"nop;nop;nop;nop;nop;nop;nop;nop;nop;nop\n"\
"nop;nop;nop;nop;nop;nop;nop;nop;nop;nop\n")

# define ISVALID(x)	((x) & 0xff00)

struct plip_private
{
	short	busy;			/* send/recv in progress */
	short	timeout;
	short	timeouts_pending;
	struct	netif *nif;		/* backlink to net if */
	
	void	(*set_strobe) (short);	/* set STROBE */
	short	(*get_busy) (void);	/* get BUSY */
	void	(*set_direction) (short);/* set data direction */
# define IN	0
# define OUT	1
	
	long	(*send_pkt) (short, char *, long);
	long	(*recv_pkt) (short, char *, long);
	
	char	(*got_ack) (void);	/* got acknowledge ? */
	void	(*send_ack) (void);	/* send acknowledge */
	void	(*cli) (void);		/* disable BUSY interrupt */
	void	(*sti) (void);		/* enable BUSY interrupt */
	
	short	(*init) (void);
	void	(*open) (void);
	void	(*close) (void);
};


# endif /* _plip_h */
