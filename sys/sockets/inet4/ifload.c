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
# include "mint/arch/asm_misc.h"


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
	
	_bpf_input:		bpf_input,

	_if_deregister:         if_deregister
};

#if 0
/*
 * This doesn't work for non-GCC compiled modules. i.e. enec.xif.
 * Looks like these modules smash other registers.
 *
 * Note: the same hack as for other kernel modules
 *       module_init() in sys/module.c
 */
static long
xif_module_init(void *initfunc, struct kerinfo *k, struct netinfo *n)
{
	long _cdecl (*init)(struct kerinfo *, struct netinfo *n);

	init = initfunc;
	return (*init)(k, n);
}
#else
static long
xif_module_init(void *initfunc, struct kerinfo *k, struct netinfo *n)
{
	register long ret __asm__("d0");

	__asm__ volatile
	(
		PUSH_SP("%%d3-%%d7/%%a3-%%a6", 36)
		"movl	%3,sp@-\n\t"
		"movl	%2,sp@-\n\t"
		"movl	%1,a0\n\t"
		"jsr	a0@\n\t"
		"addql	#8,sp\n\t"
		POP_SP("%%d3-%%d7/%%a3-%%a6", 36)
		: "=r"(ret)				/* outputs */
		: "g"(initfunc), "g"(k), "g"(n)		/* inputs  */
		: __CLOBBER_RETURN("d0")
		  "d1", "d2", "a0", "a1", "a2",	"cc",	/* clobbered regs */
		  "memory"
	);
	return ret;
}
#endif

static long
load_xif (struct basepage *b, const char *name, short *class, short *subclass)
{
#if 0
	long (*init)(struct kerinfo *, struct netinfo *);
#endif
	long r;
	
	DEBUG (("load_xif: enter (0x%lx, %s)", (unsigned long)b, name));
	DEBUG (("load_xif: init 0x%lx, size %li", (unsigned long) b->p_tbase, (b->p_tlen + b->p_dlen + b->p_blen)));
	
	/* pass a pointer to the drivers file name on to the
	 * driver.
	 */
	netinfo.fname = name;
	*class = MODCLASS_XIF;
	*subclass = 0;
#if 0
	init = (long (*)(struct kerinfo *, struct netinfo *))b->p_tbase;
	r = (*init)(KERNEL, &netinfo);
#else
	r = xif_module_init((void*)b->p_tbase, KERNEL, &netinfo);
#endif
	netinfo.fname = NULL;
	
	return r;
}

void
if_load (void)
{
	c_conws ("Loading interfaces:\r\n");
	
	if (!load_modules)
	{
		ALERT (("inet4.xdd: Loading xif's require an uptodate 1.16 kernel!"));
		return;
	}
	
	load_modules (".xif", (void *)load_xif);
}
