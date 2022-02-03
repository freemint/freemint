/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 1999, 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * 
 * Author: Frank Naumann <fnaumann@freemint.de>
 * Started: 1999-01-05
 * 
 * please send suggestions or bug reports to me or
 * the MiNT mailing list
 * 
 * 
 * changes since last version:
 * 
 * 
 * 
 * known bugs:
 * 
 * - nothing
 * 
 * todo:
 * 
 * 
 * explanation:
 * 
 * - every DMA driver first registers with the kernel. The kernel allocates a
 *   unique channel descriptor for the driver which is then used for further
 *   function calls.
 *   
 *   for registration the driver must use get_channel; get_channel return then
 *   an unique channel number; a return value of 0 indicate that there are no
 *   free channel available; any other value indicate a valid channel
 * 
 * - A DMA driver can unregister (if it wants to terminate).
 *   
 *   for unregistration the driver must use free_channel; free_channel return
 *   -1 for a channel that was not allocated; in general, this is mostly
 *   a FATAL error and the driver must halt the system after he printed
 *   some information message
 * 
 * - When entering the driver, the driver calls dma_start. The kernel
 *   automatically sets a semaphore for the driver and blocks here if another
 *   process is already using the driver.
 * 
 * - Then the driver can do whatever is necessary to start operation and
 *   enable an interrupt. When this is done, the driver calls
 *   dma_block(channel) which will block the current process and give up the
 *   processor.
 *   
 *   dma_block can also automatically handle timeout events. For this dma_block
 *   get an timeout value. The value specifiy the time in milliseconds.
 *   If the timeout expire it will call the function that is the third argument
 *   to dma_block or a default timeout function if the third argument is NULL.
 *   The default timeout function simply call deblock(channel, -2) to unblock
 *   the process. The return value of dma_block is then -2 if the timeout
 *   deblock the process.
 *   If you doesn't need a timeout simply set all other arguments of dma_block
 *   to 0. A value of 0 for the timeout means an infinite timeout that
 *   effectivly disable this feature.
 *   
 *   Note: It's not granted that the timeout happen *exactly* after the
 *         specified number of milliseconds.
 * 
 * - Now another process can run; if this process also tries to access the
 *   driver, it will automatically be blocked because the semaphore is locked.
 * 
 * - When the device has finished its operation, it causes an interrupt which
 *   calls the ISR in the driver. The driver can either restart the device (if
 *   further processing is required for the current task, eg. when doing
 *   scatter-gather DMA), *or* it can call dma_deblock(channel). In either
 *   case, the ISR ends and control returns to the process that was running
 *   when the interrupt happened.  A call to dma_deblock(channel) unblocks the
 *   driver task. The kernel then switches back to the driver task, which
 *   continues by returning from dma_block. The driver may now either restart
 *   the device (using operations that take too long to do this in the ISR, or
 *   that need kernel calls) and call dma_block again, or it finishes by
 *   cleaning up and calling dma_end.
 *   
 *   Note: After dma_start the interrupt can happen at any time and call
 *         dma_deblock. The dma_block/dma_deblock interaction is interrupt
 *         safe and dma_block will not block in such case and return
 *         immediate.
 *   
 *   Note: dma_deblock get an additional parameter, called idev, 32bit long;
 *         this is simply and internal driver value that is returned unchanged
 *         from dma_block as return value
 *         if your driver doesn't need an additional value simply set it to 0;
 *         dma_block return then 0
 * 
 * - When leaving the driver, the driver calls dma_end. This resets the
 *   semaphore and unblocks other processes (one at a time) waiting to enter
 *   the driver.
 * 
 * - dma_deblock is the only routine that may be called from within an
 *   interrupt service routine.
 * 
 */

# include "dma.h"

# include "info.h"
# include "proc.h"
# include "timeout.h"


# define DMA_VERS	1		/* internal version */

# ifdef DEBUG_INFO
# define DMA_DEBUG(x)	FORCE x
# else
# define DMA_DEBUG(x)
# endif

/****************************************************************************/
/* BEGIN definition part */

static ulong	_cdecl dma_get_channel	(void);
static long	_cdecl dma_free_channel	(ulong i);
static void	_cdecl dma_start	(ulong i);
static void	_cdecl dma_end		(ulong i);
static void *	_cdecl dma_block	(ulong i, ulong timeout, void _cdecl (*func)(PROC *p, long arg));
static void	_cdecl dma_deblock	(ulong i, void *idev);

DMA dma =
{
	dma_get_channel,
	dma_free_channel,
	dma_start,
	dma_end,
	dma_block,
	dma_deblock
};

/* END definition part */
/****************************************************************************/

/****************************************************************************/
/* BEGIN global data */

typedef struct channel CHANNEL;

# define CHANNELS 64
struct channel
{
	volatile ushort	lock;
	volatile ushort	sleepers;
	ushort		pid;
	ushort		used;
	PROC		*p;		/* PROC for the blocked process */
	TIMEOUT		*t;		/* contain timeout ptr */
	void		*idev;		/* uwe really like types :-) */
};

static CHANNEL channels [CHANNELS];

/* END global data */
/****************************************************************************/

/****************************************************************************/
/* BEGIN  */

static ulong _cdecl
dma_get_channel (void)
{
	ulong i;
	
	for (i = 0; i < CHANNELS; i++)
	{
		if (!(channels[i].used))
		{
			channels[i].lock	=  0;
			channels[i].sleepers	=  0;
			channels[i].pid		= -1;
			channels[i].used	=  1;
			channels[i].p		=  NULL;
			channels[i].t		=  NULL;
			channels[i].idev	=  NULL;
			
			DMA_DEBUG (("dma_get_channel: return %lu", i+1));
			return ++i;
		}
	}
	
	DMA_DEBUG (("dma_get_channel: return 0"));
	return 0L;
}

static long _cdecl
dma_free_channel (ulong i)
{
	CHANNEL *c;
	
	DMA_DEBUG (("dma_free_channel: enter (%lu)", i));
	
	if (--i >= CHANNELS)
		return -1L;
	
	c = &channels[i];
	if (!c->used)
		return -1L;
	
	c->used = 0;
	return 0L;
}

/* only synchronusly callable [to kernel]
 */
static void _cdecl
dma_start (ulong i)
{
	CHANNEL *c;
	
	DMA_DEBUG (("dma_start: enter (%lu)", i));
	
	if (--i >= CHANNELS)
		FATAL (ERR_dma_start_on_inv_handle, i+1);
	
	c = &channels[i];
	if (!c->used)
		FATAL (ERR_dma_start_on_inv_handle, i+1);
	
	while (c->lock)
	{
		DMA_DEBUG (("dma_start: semaphore locked, sleeping (%lu)", i+1));
		
		c->pid = -1;
		
		c->sleepers++;
		sleep (IO_Q, (long) c);
		c->sleepers--;
	}
	
	c->lock = 1;
	c->pid = get_curproc()->pid;
	c->p = get_curproc();
}

/* only synchronusly callable [to kernel]
 */
static void _cdecl
dma_end (ulong i)
{
	CHANNEL *c;
	
	DMA_DEBUG (("dma_end: enter (%lu)", i));
	
	if (--i >= CHANNELS)
		FATAL (ERR_dma_end_on_inv_handle, i+1);
	
	c = &channels[i];
	if (!c->used)
		FATAL (ERR_dma_end_on_inv_handle, i+1);
	
	if (!c->lock)
		FATAL (ERR_dma_end_on_unlocked_handle, i+1);
	
	c->lock = 0;
	c->pid = -1;
	c->p = NULL;
	
	if (c->sleepers)
		wake (IO_Q, (long) c);
}

static void _cdecl
dma_timeout (PROC *p, long i)
{
	CHANNEL *c = &channels[i];
	
	c->t = NULL;
	dma_deblock (i + 1, (void *) -2);
}

/* only synchronusly callable [to kernel]
 */
static void * _cdecl
dma_block (ulong i, ulong timeout, void _cdecl (*func)(PROC *p, long arg))
{
	CHANNEL *c;
	
	DMA_DEBUG (("dma_block: enter (%lu)", i));
	
	if (--i >= CHANNELS)
		FATAL (ERR_dma_block_on_inv_handle, i+1);
	
	c = &channels[i];
	if (!c->used)
		FATAL (ERR_dma_block_on_inv_handle, i+1);
	
	if (timeout)
	{
		c->t = addroottimeout (timeout,
				func ? func
				: dma_timeout, 0);
		if (!c->t)
			FATAL (ERR_dma_addroottimeout);
		
		c->t->arg = i;
	}
	
	sleep (IO_Q, (long) &(c->pid));
	c->pid = get_curproc()->pid;
	
	if (c->t)
	{
		cancelroottimeout (c->t);
		c->t = NULL;
	}
	
	DMA_DEBUG (("dma_block: leave (%lu -> %p)", i+1, c->idev));
	return c->idev;
}

/* synchronus/asynchronus callable
 */
static void _cdecl
dma_deblock (ulong i, void *idev)
{
	CHANNEL *c;
	
	if (--i >= CHANNELS)
		FATAL (ERR_dma_deblock_on_inv_handle, i+1);
	
	c = &channels[i];
	if (!c->used)
		FATAL (ERR_dma_deblock_on_inv_handle, i+1);
	
	c->idev = idev;
	iwake (IO_Q, (long) &(c->pid), c->pid);
}

/* END  */
/****************************************************************************/

/****************************************************************************/
/* BEGIN  */

/* END  */
/****************************************************************************/
