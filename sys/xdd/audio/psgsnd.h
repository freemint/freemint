/*
 * Yamaha PSG registers
 */

# ifndef _psgsnd_h
# define _psgsnd_h

extern const unsigned short digitab[];

# define SIZEOF_DENTRY	4 /* * sizeof (short) */

# define PSGselect	(*(volatile unsigned char *) 0xffff8800L)
# define  PSG_FREQ1A	0
# define  PSG_FREQ2A	1
# define  PSG_FREQ1B	2
# define  PSG_FREQ2B	3
# define  PSG_FREQ1C	4
# define  PSG_FREQ2C	5
# define  PSG_NOISE	6
# define  PSG_MIX	7
# define   PSG_ALL_OFF	0x3f
# define  PSG_AMPA	8
# define  PSG_AMPB	9
# define  PSG_AMPC	10
# define  PSG_FREQ1H	11
# define  PSG_FREQ2H	12
# define  PSG_HULL	13
# define  PSG_IOA	14
# define  PSG_IOB	15

# define PSGwrite	(*(volatile unsigned char *) 0xffff8802L)
# define PSGread	(*(volatile unsigned char *) 0xffff8800L)

# define PSGplay(v1,v2,v3) __asm__ volatile ( \
	"movepw %0, %3@(0)\n" \
	"movepw %1, %3@(0)\n" \
	"movepw %2, %3@(0)\n" \
	:: "d"(v1), "d"(v2), "d"(v3), "a" (0xffff8800L))
 
# endif /* _psgsnd_h */
