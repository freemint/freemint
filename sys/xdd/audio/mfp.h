/*
 * Some inlines dealing with MFP timers
 *
 * 11/03/95, Kay Roemer.
 */

# ifndef _mfp_h
# define _mfp_h

# include "mint/asm.h"


# define MFP_TADR	(*(volatile unsigned char *)0xfffffa1fL)
# define MFP_TACR	(*(volatile unsigned char *)0xfffffa19L)
# define MFP_TBDR	(*(volatile unsigned char *)0xfffffa21L)
# define MFP_TBCR	(*(volatile unsigned char *)0xfffffa1bL)

# define MFP_IERA	(*(volatile unsigned char *)0xfffffa07L)
# define MFP_IPRA	(*(volatile unsigned char *)0xfffffa0bL)
# define MFP_IMRA	(*(volatile unsigned char *)0xfffffa13L)

# define MFP_CLOCK	2457600L

/*
 * values for MFP Timer A/B control register
 */
# define MPF_STOP	0
# define MFP_DIV4	1
# define MFP_DIV10	2
# define MFP_DIV16	3
# define MFP_DIV50	4
# define MFP_DIV64	5
# define MFP_DIV100	6
# define MFP_DIV20	7

static inline void
timera_enable (unsigned char ctrl)
{
	MFP_TACR = ctrl;
}

static inline void
timera_disable (void)
{
	MFP_TACR = 0;
}

static inline void
timer_cli (unsigned char mask)
{
	register ushort sr;
	
	while (MFP_IPRA & mask)
		;
	
	sr = splhigh ();
	MFP_IMRA &= ~mask;
	spl (sr);
}

static inline void
timer_sti (unsigned char mask)
{
	register ushort sr = splhigh ();
	MFP_IMRA |= mask;
	spl (sr);
}

# define timera_cli()	(timer_cli (0x20))
# define timera_sti()	(timer_sti (0x20))
# define timerb_cli()	(timer_cli (0x01))
# define timerb_sti()	(timer_sti (0x01))
# define gpi7_cli()	(timer_cli (0x80))
# define gpi7_sti()	(timer_sti (0x80))


# endif /* _mfp_h */
