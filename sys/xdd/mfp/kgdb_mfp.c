
#include "mint/mint.h"
#include "mfp.h"

#include "mint/arch/asm.h"


static struct mfp *regs = _mfpregs;

INLINE void
rts_on(void)
{
#ifdef MILAN
	/* regs->gpip &= ~GPIP_RTS; */	/* RTS (bit0) = 0 */
	asm volatile
	(
		"bclr.b #0,3(%0)"
		:
		: "a" (regs)
		: "cc"
	);
#else
	*_giselect = 14;
	*_giwrite = *_giread & ~GI_RTS;
#endif
}

INLINE void
rts_off(void)
{
#ifdef MILAN
	/* regs->gpip |= GPIP_RTS; */	/* RTS (bit0) = 1 */
	asm volatile
	(
		"bset.b #0,3(%0)"
		:
		: "a" (regs)
		: "cc"
	);
#else
	*_giselect = 14;
	*_giwrite = *_giread | GI_RTS;
#endif
}


INLINE void
dtr_on(void)
{
#ifdef MILAN
	/* regs->gpip &= ~GPIP_DTR; */
	asm volatile
	(
		"bclr.b #3,3(%0)"
		:
		: "a" (regs)
		: "cc"
	);
#else
	*_giselect = 14;
	*_giwrite = *_giread & ~GI_DTR;
#endif
}

INLINE void
dtr_off(void)
{
#ifdef MILAN
	/* regs->gpip |= GPIP_DTR; */
	asm volatile
	(
		"bset.b #3,3(%0)"
		:
		: "a" (regs)
		: "cc"
	);
#else
	*_giselect = 14;
	*_giwrite = *_giread | GI_DTR;
#endif
}

static void
mfp_kgdb_init(void)
{
	unsigned char reg;

	{
		static short minit = 0;

		if (minit)
			return;

		minit = 1;
	}

	/* reset the MFP to default values */
	regs->ucr = UCR_PREDIV | UCR_ASYNC_1;
	regs->rsr = 0x00;
	regs->tsr = 0x00;

	/* set standard baudrate */
	regs->tcdcr &= 0x70;
	regs->tddr = 0x01;
	regs->tcdcr |= 1;

	/* disable interrupts */
	reg = regs->iera;
	reg &= ~(0x2 | 0x4 | 0x8 | 0x10);
	regs->iera = reg;

	reg = regs->ierb;
	reg &= ~(0x2 | 0x4);
	regs->ierb = reg;

	/* disable interrupt mask */
	reg = regs->imra;
	reg &= ~(0x2 | 0x4 | 0x8 | 0x10);
	regs->imra = reg;

	reg = regs->imrb;
	reg &= ~(0x2 | 0x4);
	regs->imrb = reg;

	/* enable rx/tx */
	regs->rsr = RSR_RX_ENAB;
	regs->tsr = TSR_TX_ENAB;

	dtr_on();
	rts_on();
}

static int
mfp_kgdb_instat(void)
{
	unsigned short sr = splhigh();
	unsigned char rsr;

	mfp_kgdb_init();

	rsr = regs->rsr;

	spl(sr);

	return (rsr & RSR_CHAR_AVAILABLE);
}

static int
mfp_kgdb_getc(void)
{
	unsigned short sr = splhigh();
	unsigned char rsr;
	int udr;

	mfp_kgdb_init();

	rsr = regs->rsr;
	while (!(rsr & RSR_CHAR_AVAILABLE))
		rsr = regs->rsr;

	udr = regs->udr;

	spl(sr);

	return udr;
}

static void
mfp_kgdb_putc(int c)
{
	unsigned short sr = splhigh();
	unsigned char tsr;

	mfp_kgdb_init();

	tsr = regs->tsr;
	while (!(tsr & TSR_BUF_EMPTY))
		tsr = regs->tsr;

	regs->udr = c;
	spl(sr);
}
