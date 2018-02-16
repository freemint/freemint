/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 * 
 * 
 * Include file for the random number generator.
 * 
 */

# ifndef _random_h
# define _random_h

# include "mint/mint.h"


# ifdef DEV_RANDOM

/* ioctl()'s for the random number generator.
 * They will hopefully not conflict.
 */

/* Get the entropy count. */
# define RNDGETENTCNT	(('R' << 8) | 0)

/* Add to (or subtract from) the entropy count.  (Superuser only.) */
# define RNDADDTOENTCNT	(('R' << 8) | 1)

/* Get the contents of the entropy pool.  (Superuser only.) */
# define RNDGETPOOL	(('R' << 8) | 2)

/* 
 * Write bytes into the entropy pool and add to the entropy count.
 * (Superuser only.)
 */
# define RNDADDENTROPY	(('R' << 8) | 3)

/* Clear entropy count to 0.  (Superuser only.) */
# define RNDZAPENTCNT	(('R' << 8) | 4)

/* Clear the entropy pool and associated counters.  (Superuser only.) */
# define RNDCLEARPOOL	(('R' << 8) | 5)

struct rand_pool_info {
	long	entropy_count;
	long	buf_size;
	ulong	buf[0];
};

/* Exported functions */

# ifdef __KERNEL__

void rand_initialize (void);
void rand_initialize_irq (int irq);
void rand_initialize_blkdev (int major);

void add_keyboard_randomness (unsigned short scancode);
void add_mouse_randomness (ulong mouse_data);
void add_interrupt_randomness (int irq);
void add_blkdev_randomness (int major);

void get_random_bytes (void *buf, unsigned long nbytes);

# ifdef DO_IT_LATER
/* These functions could be exported if need be.  */
ulong secure_tcp_sequence_number(ulong saddr, ulong daddr,
					ushort sport, ushort dport);
ulong secure_tcp_syn_cookie(ulong saddr, ulong daddr,
				   ushort sport, ushort dport,
				   ulong sseq, ulong count,
				   ulong data);
ulong check_tcp_syn_cookie(ulong cookie, ulong saddr,
				  ulong daddr, ushort sport,
				  ushort dport, ulong sseq,
				  ulong count, ulong maxdiff);
# endif

extern DEVDRV random_device, urandom_device;
void checkrandom (void);

# endif /* __KERNEL___ */

# endif /* DEV_RANDOM */


# endif /* _random_h */
