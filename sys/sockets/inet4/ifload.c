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

# include <mint/basepage.h>
# include <mint/config.h>


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

char *paths[] = { "\\mint\\", "\\multitos\\" };

typedef long (*XIFFUNC)(struct kerinfo *, struct netinfo *);

void
if_load (void)
{
	static char oldpath[PATH_MAX];
	static DTABUF dta;
	DTABUF *olddta;
	BASEPAGE *bp;
	XIFFUNC initf;
	short i;
	long r;
	
	olddta = (DTABUF *) f_getdta ();
	d_getpath (oldpath, 0);
	f_setdta (&dta);
	
	/*
	 * pass a pointer to the drivers file name on to the
	 * driver.
	 */
	netinfo.fname = dta.dta_name;
	
	c_conws ("Loading interfaces:\n\r");
	
	for (i = 0; i < sizeof (paths)/sizeof (*paths); ++i)
	{
		r = d_setpath (paths[i]);
		if (r == 0)
			r = f_sfirst ("*.xif", 0);
		
		while (r == 0)
		{
			bp = (BASEPAGE *) p_exec (3, dta.dta_name, "", 0);
			if ((long)bp < 0)
			{
				DEBUG (("if_load: can't load %s", dta.dta_name));
				r = f_snext ();
				continue;
			}
			
			m_shrink (0, (long) bp, 512+bp->p_tlen+bp->p_dlen+bp->p_blen);
			initf = (XIFFUNC) bp->p_tbase;
			TRACE (("if_load: init %s", dta.dta_name));
			if ((*initf) (KERNEL, &netinfo) != 0)
			{
				DEBUG (("if_load: init %s failed", dta.dta_name));
				m_free (bp);
			}
			TRACE (("if_load: init %s ok.", dta.dta_name)); 
			r = f_snext ();
		}
	}
	
	d_setpath (oldpath);
	f_setdta (olddta);
}
