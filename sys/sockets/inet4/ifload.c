/*
 *	Load network interfaces (*.xif) from /mint/ and /multitos/.
 *
 *	06/22/94, Kay Roemer.
 */

# include "ifload.h"

/* Keep netinfo.h from defining it */
# define NETINFO
# include "netinfo.h"

# include "bpf.h"
# include "inet.h"
# include "inetutil.h"
# include "ifeth.h"

# include "mint/basepage.h"
# include "mint/config.h"
# include "mint/emu_tos.h"


struct netinfo netinfo =
{
	_buf_alloc:		buf_alloc,
	_buf_free:		buf_free,
	_buf_reserve:		buf_reserve,
	_buf_deref:		buf_deref,
	
	_if_enqueue:		if_enqueue,
	_if_dequeue:		if_dequeue,
	_if_register:		if_register,
	_if_input:		if_input,
	_if_flushq:		if_flushq,
	
	_in_chksum:		chksum,
	_if_getfreeunit:	if_getfreeunit,
	
	_eth_build_hdr:		eth_build_hdr,
	_eth_remove_hdr:	eth_remove_hdr,
	
	fname:			NULL,
	
	_bpf_input:		bpf_input
};


static long
callout_init (void *initfunction, struct kerinfo *k, struct netinfo *n)
{
	register long ret __asm__("d0");
	
	__asm__ volatile
	(
		"moveml d3-d7/a3-a6,sp@-;"
		"movl	%3,sp@-;"
		"movl	%2,sp@-;"
		"movl   %1,a0;"
		"jsr    a0@;"
		"addqw  #8,sp;"
		"moveml sp@+,d3-d7/a3-a6;"
		: "=r"(ret)				/* outputs */
		: "g"(initfunction), "r"(k), "r"(n)	/* inputs  */
		: "d0", "d1", "d2", "a0", "a1", "a2",   /* clobbered regs */
		  "memory"
	);
	
	return ret;
}

static long
load_xif (struct basepage *b, const char *name)
{
	long r;
	
	DEBUG (("load_xif: enter (0x%lx, %s)", b, name));
	DEBUG (("load_xif: init 0x%lx, size %li", (void *) b->p_tbase, (b->p_tlen + b->p_dlen + b->p_blen)));
	
	/* pass a pointer to the drivers file name on to the
	 * driver.
	 */
	netinfo.fname = name;
	
	r = callout_init ((void *) b->p_tbase, KERNEL, &netinfo);
	
	netinfo.fname = NULL;
	
	return r;
}

void
if_load (void)
{
	c_conws ("Loading interfaces:\r\n");
	
	if (load_modules)
		load_modules (".xif", load_xif);
	else
		FATAL ("This version require a newer kernel!");
}
