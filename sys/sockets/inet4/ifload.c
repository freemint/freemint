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
xif_module_init(void *initfunc, struct kerinfo *k, struct netinfo *n)
{
	long _cdecl (*init)(struct kerinfo *, struct netinfo *n);

	init = initfunc;
	return (*init)(k, n);
}

static long
load_xif (struct basepage *b, const char *name, short *class, short *subclass)
{
	long r;
	
	DEBUG (("load_xif: enter (0x%lx, %s)", b, name));
	DEBUG (("load_xif: init 0x%lx, size %li", (void *) b->p_tbase, (b->p_tlen + b->p_dlen + b->p_blen)));
	
	/* pass a pointer to the drivers file name on to the
	 * driver.
	 */
	netinfo.fname = name;
	*class = MODCLASS_XIF;
	*subclass = 0;
	r = xif_module_init((void*)b->p_tbase, KERNEL, &netinfo);
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
