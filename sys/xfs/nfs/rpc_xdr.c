/*
 * Copyright 1993, 1994 by Ulrich K�hn. All rights reserved.
 *
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 * 
 * Modified for FreeMiNT CVS
 * by Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
 */

/*
 * File : rpc.c
 *        some utility functions for rpc
 */


#include "rpc_xdr.h"

# ifndef NULL
# define NULL	(0L)
# endif


/* a library-wide null authentificator */
struct opaque_auth null_auth = { AUTH_NULL, NULL, 0 };



long
xdr_size_opaque_auth(opaque_auth *ap)
{
	long r = 2*sizeof(ulong);

	r += ap->len;
	return r;
}


bool_t
xdr_opaque_auth(xdrs *x, opaque_auth *ap)
{
	const char *name;
	xdr_enum(x, &ap->flavor);
	name = ap->data;
	return xdr_opaque(x, &name, (long *)&ap->len, MAX_AUTH_BYTES);
}


long
xdr_size_rpc_msg(rpc_msg *m)
{
	long len;

	len = sizeof(ulong)+sizeof(ulong);   /* xid, mtype */
	switch (m->mtype)
	{
		case CALL:
			len += 4*sizeof(ulong);
			len += 2*(sizeof(ulong)+sizeof(ulong));  /* two opaque_auth fields */
			if ((m->cbody.cred.len > MAX_AUTH_BYTES) ||
			          (m->cbody.verf.len > MAX_AUTH_BYTES))
				return 0;
			len += m->cbody.cred.len;
			len += m->cbody.verf.len;
			break;
		case REPLY:
			len += sizeof(ulong);
			switch (m->rbody.rb_stat)
			{
				case MSG_ACCEPTED:
					len += sizeof(ulong)+sizeof(ulong);  /* opaque_auth */
					if (m->rbody.rb_arpl.ar_verf.len > MAX_AUTH_BYTES)
						return 0;
					len += m->rbody.rb_arpl.ar_verf.len;
					len += sizeof(ulong);
					if (PROG_MISMATCH == m->rbody.rb_arpl.ar_stat)
						len += 2*sizeof(ulong);
					break;
				case MSG_DENIED:
					len += sizeof(ulong);
					if (RPC_MISMATCH == m->rbody.rb_rrpl.rr_stat)
						len += 2*sizeof(ulong);
					else if (AUTH_ERROR == m->rbody.rb_rrpl.rr_stat)
						len += sizeof(ulong);
					break;
			}
			break;
	}
	return len;
}


bool_t
xdr_rpc_msg(xdrs *x, rpc_msg *m)
{
	xdr_ulong(x, &m->xid);
	xdr_enum(x, &m->mtype);

	switch (m->mtype)
	{
		case CALL:
			xdr_ulong(x, &m->cbody.rpcvers);
			xdr_ulong(x, &m->cbody.prog);
			xdr_ulong(x, &m->cbody.vers);
			xdr_ulong(x, &m->cbody.proc);
			if (!xdr_opaque_auth(x, &m->cbody.cred))  return FALSE;
			if (!xdr_opaque_auth(x, &m->cbody.verf))  return FALSE;
			if (m->cbody.xproc)
			{
				if (!(*(m->cbody.xproc))(x, m->cbody.data))
					return FALSE;
			}
			break;
		case REPLY:
			xdr_enum(x, &m->rbody.rb_stat);
			if (MSG_ACCEPTED == m->rbody.rb_stat)
			{
				if (!xdr_opaque_auth(x, &m->rbody.rb_arpl.ar_verf))
					return FALSE;
				xdr_enum(x, &m->rbody.rb_arpl.ar_stat);
				if (PROG_MISMATCH == m->rbody.rb_arpl.ar_stat)
				{
					xdr_ulong(x, &m->rbody.rb_arpl.ar_mis_info.low);
					xdr_ulong(x, &m->rbody.rb_arpl.ar_mis_info.high);
				}
				if (SUCCESS == m->rbody.rb_arpl.ar_stat)
				{
					if (m->rbody.rb_arpl.ar_result.xproc)
					{
						if (!(*(m->rbody.rb_arpl.ar_result.xproc))
						                       (x, m->rbody.rb_arpl.ar_result.data))
							return FALSE;
					}
				}
			}
			else if (MSG_DENIED == m->rbody.rb_stat)
			{
				xdr_enum(x, &m->rbody.rb_rrpl.rr_stat);
				if (RPC_MISMATCH == m->rbody.rb_rrpl.rr_stat)
				{
					xdr_ulong(x, &m->rbody.rb_rrpl.rr_mis_info.low);
					xdr_ulong(x, &m->rbody.rb_rrpl.rr_mis_info.high);
				}
				else if (AUTH_ERROR == m->rbody.rb_rrpl.rr_stat)
					xdr_enum(x, &m->rbody.rb_rrpl.rr_astat);
			}
			break;
		default:
			return FALSE;
	}
	return TRUE;
}
