/*
 * File: svc_auth.c
 *       authenticate a service call, so far only auth_null and auth_unix
 */

#include "xdr.h"
#include "rpc.h"
#include "svc.h"
#include "auth_unix.h"



enum auth_stat authenticate(struct svc_req *req, struct rpc_msg *msg)
{
	req->rq_cred = msg->cbody.cred;
	req->rq_xprt->xp_verf = null_auth;

	if (msg->cbody.cred.flavor == AUTH_NULL)
	{
		return 0;
	}
	if (msg->cbody.cred.flavor == AUTH_UNIX)
	{
		auth_unix *au = (auth_unix*)req->rq_clntcred;
		xdrs x;
		char *s = &au->au_machname[0];
		long *buf, i;

		xdr_init(&x, req->rq_cred.data, MAX_AUTH_BYTES, XDR_DECODE, NULL);
		if (!xdr_ulong(&x, &au->au_time))
			return AUTH_BADCRED;
		if (!xdr_string(&x, &s, MAXMACHINENAME))
			return AUTH_BADCRED;
		buf = xdr_inline(&x, sizeof(long)*3);
		if (!buf)
			return AUTH_BADCRED;
		au->au_uid = IXDR_GET_LONG(buf);
		au->au_gid = IXDR_GET_LONG(buf);
		au->au_len = IXDR_GET_ULONG(buf);
		if (au->au_len > NGRPS)
			au->au_len = NGRPS;
		buf = xdr_inline(&x, sizeof(long)*au->au_len);
		if (!buf)
			return AUTH_BADCRED;
		for (i = 0;  i < au->au_len;  i++)
			au->au_gids[i] = IXDR_GET_LONG(buf);

		return 0;
	}

	/* other stuff not implemented */
	return AUTH_REJECTEDCRED;
}
