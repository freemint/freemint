#ifndef _DE600_H
#define _DE600_H

/*
 * Adapter base address.
 *
 * A byte `c' is written to the DATA port by reading the address
 * DE600_BASE + (c << 1). The DATA port looks like:
 *
 * bit #	centronix wire(pin)
 * 0		D0(2)
 * 1		D1(3)
 * 2		D2(4)
 * 3		D3(5)
 * 4		D4(6)
 * 5		D5(7)
 * 6		D6(8)
 * 7		D7(9)
 *
 * The STATUS register is read by reading one of the addresses
 * DE600_BASE .. (DE600_BASE + 0x1ff). The STATUS port looks like:
 *
 * bit #	centronix wire(pin)
 * 0-2		not connected, always zero
 * 3		Error(15)
 * 4		Select(13)
 * 5		PaperOut(12)
 * 6		Ack(10)
 * 7		Busy(11)
 *
 * Only bit 3 of the COMMAND register can be accessed by reading the address
 * DE600_BASE + (b << 9) where `b' is bit # 3. All other bits are hard-wired.
 * The COMMAND port looks like
 *
 * bit #	centronix wire(pin)	hard-wired to
 * 0		Strobe(1)		Vcc
 * 1		Autofeed(14)		Vcc
 * 2		Reset(16)		Vcc
 * 3		Selected(17)
 * 4		Let Ack cause interrupt
 * 5-7		not used
 */
#define DE600_BASE	0xfffa0201L

/*
 * Number of send pages
 */
#define TX_PAGES	2

/*
 * DE600 status register contents, read out of STATUS port
 */
#define RX_BUSY		0x80
#define RX_GOOD		0x40
#define TX_FAILED16	0x10
#define TX_BUSY		0x08

/*
 * DE600 DATA port commands, command is passed in low nibble, data in
 * high nibble. High data nibble is marked with HI_NIBBLE.
 */
#define S(x)		((x) << 1)

#define WRITE_DATA	S(0x00)	/* write to mem */
#define READ_DATA	S(0x01)	/* read from mem */
#define READ_STAT	S(0x02)	/* read status register */
#define NULL_CMD	S(0x04)	/* null command */
#define RX_LEN		S(0x05)	/* read received packet length */
#define TX_ADDR		S(0x06)	/* set adapter transmit mem address */
#define RW_ADDR		S(0x07)	/* set adapter read/write mem address */
#define HI_NIBBLE	S(0x08)	/* or with this if high data nibble is sent */

#define WRITE_CMD	S(0x03)	/* write to command register */

/*
 * Command register data, high nibble for WRITE_CMD.
 */
#define RX_NON		0x00	/* receive nothing */
#define RX_ALL		0x01	/* receive all addr */
#define RX_BP		0x02	/* receive my and broadcast addr */
#define RX_MBP		0x03	/* receive my, broadcast and multicast addr */
#define TX_ENABLE	0x04	/* enable sending */
#define RX_ENABLE	0x08	/* enable receiving */
#define RESET_ON	0x80	/* reset adapter */
#define RESET_OFF	0x00	/* end reset */
#define RX_PAGE2	0x10	/* select 2nd rx page */
#define RX_PAGE1	0x20	/* select 1st rx page */
#define FLIP_IRQ	0x40	/* toggle interrupt */

/*
 * DE600 memory layout:
 * 0-2k: 1st xmit page (send from pointer to 2k)
 * 2-4k: 2nd xmit page (send from pointer to 4k)
 * 4-6k: 1st recv page (recv up to pointer)
 * 6-8k: 2nd recv page (recv up to pointer)
 * 8k:   3 bytes magic code 0x00de15 and last 3 bytes of enet address
 */
#define MEM_0K		0x0000
#define MEM_2K		0x0800
#define MEM_4K		0x1000
#define MEM_6K		0x1800
#define MEM_ROM		0x2000

/*
 * Min enet frame length
 */
#define MIN_LEN		60

/*
 * assembler stuff from de600asm.s
 */
extern void	de600_cli (void);
extern void	de600_sti (void);
extern void de600_dints (void);
extern void	de600_busy_int (void);
extern long	de600_old_busy_int;

/*
 * D600 I/O routines. Some broken compilers ignore the volatile and
 * optimize the below routines to mostly nothing. Be careful!
 *
 * The following functions rely on the compiler to preload de600_base
 * into an address register and to access the adapter via addressing
 * mode `address register indirect with register offset'. Gcc does
 * this, for other compilers have a look at the assembly output!
 *
 * For efficiency's sake these are #define's rather than inlines, because
 * gcc produces better code this way.
 */
static volatile unsigned char *de600_base = (unsigned char *)DE600_BASE;

#if 0
static unsigned char	recv_byte (u_short);
static unsigned char	recv_stat (void);
static void 		send_byte (u_short, u_short);
static void		send_cmd  (u_short, u_short);
static void		send_addr (u_short, u_short);
static void		select_prn (void);
static void		select_nic (void);
#endif

static void		recv_block (char *, short);
static void		send_block (char *, short);

/*
 * Hard wait
 */
#define wait()	__asm__ volatile ("nop;nop;nop;nop;nop;nop;nop;nop\n")

/*
 * Extract nibbles from short and shift them 1 to the left
 */
#define nib0(x)	(((unsigned short)(x) << 5) & 0x1e0)
#define nib1(x) (((unsigned short)(x) << 1) & 0x1e0)
#define nib2(x)	(((unsigned short)(x) >> 3) & 0x1e0)
#define nib3(x) (((unsigned short)(x) >> 7) & 0x1e0)

/*
 * send a byte to the printer DATA register
 */
#define send_byte(cmd, c) { \
	(void)de600_base[nib0 (c) | (cmd)]; \
	wait (); \
	(void)de600_base[nib1 (c) | (cmd) | HI_NIBBLE]; \
	wait (); \
}

/*
 * read a byte from the printer STATUS register
 */
#define recv_byte(cmd) ({ \
	unsigned char c; \
	(void)de600_base[(cmd)]; \
	wait (); \
	c =   de600_base[(cmd) | HI_NIBBLE] >> 4; \
	wait (); \
	c |=  de600_base[(cmd) | HI_NIBBLE]; \
})

/*
 * read DE600 status register
 */
#define recv_stat() ({ \
	(void)de600_base[READ_STAT]; \
	wait (); \
	de600_base[NULL_CMD | HI_NIBBLE]; \
})

/*
 * write a byte to the DE600 command register, `rxpage' is the high nibble
 * of the command byte, `cmd' the low nibble.
 */
#define send_cmd(rxpage, cmd) { \
	(void)de600_base[nib0 (rxpage) | WRITE_CMD]; \
	wait (); \
	(void)de600_base[nib1 (rxpage) | WRITE_CMD | HI_NIBBLE]; \
	wait (); \
	(void)de600_base[nib0 ((rxpage)|(cmd)) | WRITE_CMD]; \
	wait (); \
	(void)de600_base[nib1 ((rxpage)|(cmd)) | WRITE_CMD | HI_NIBBLE]; \
	wait (); \
}

/*
 * write a DE600 internal memory address to the DE600
 */
#define send_addr(cmd, addr) { \
	(void)de600_base[nib0 (addr) | (cmd)]; \
	wait (); \
	(void)de600_base[nib1 (addr) | (cmd) | HI_NIBBLE]; \
	wait (); \
	(void)de600_base[nib2 (addr) | (cmd)]; \
	wait (); \
	(void)de600_base[nib3 (addr) | (cmd) | HI_NIBBLE]; \
	wait (); \
}

/*
 * Select printer be setting SELECTED wire to high.
 */
#define select_prn() { \
	(void)de600_base[NULL_CMD | 0x200]; \
	wait (); \
	(void)de600_base[NULL_CMD | 0x200 | HI_NIBBLE]; \
	wait (); \
}

/*
 * Select network interface card by setting SELECTED to low.
 */
#define select_nic() { \
	(void)de600_base[NULL_CMD]; \
	wait (); \
	(void)de600_base[NULL_CMD | HI_NIBBLE]; \
	wait (); \
}

/*
 * Read 'len' bytes from DE600 mem to 'cp'. Caller must ensure 'len' > 1.
 */
static inline void
recv_block (char *cp, short len)
{
	__asm__ volatile (
		"tstb	%2@(2)		\n"
		"moveb	%2@(18), %%d0	\n"
		"lsrb	#4, %%d0		\n"
		"subqw	#2, %0		\n"
		"1:			\n"
		"orb	%2@(2), %%d0	\n"
		"moveb	%%d0, %1@+	\n"
		"moveb	%2@(18), %%d0	\n"
		"lsrb	#4, %%d0		\n"
		"dbra	%0, 1b		\n"
		"orb	%2@(18), %%d0	\n"
		"moveb	%%d0, %1@		\n"
		: "=d"(len), "=a"(cp)
		: "a"(de600_base), "0"(len), "1"(cp)
		: "d0");
}

/*
 * Write `len' bytes from `cp' to DE600. Caller must ensure 'len' > 0.
 */
static inline void
send_block (char *cp, short len)
{
	__asm__ volatile (
		"subqw	#1, %0		\n"
		"2:			\n"
		"moveb	%1@+, %%d0	\n"
		"addw	%%d0, %%d0		\n"
		"movew	%%d0, %%d1		\n"
		"lslw	#4, %%d1		\n"
		"andw	%5, %%d1		\n"
		"tstb	%2@(%%d1:w)	\n"
		"andw	%5, %%d0		\n"
		"bset	#4, %%d0		\n"
		"tstb	%2@(%%d0:w)	\n"
		"dbra	%0, 2b		\n"
		: "=d"(len), "=a"(cp)
		: "a"(de600_base), "0"(len), "1"(cp), "d"(0x1e0)
		: "d0", "d1");
}
#endif
