/*
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 *
 * A major inspiration was the SUNRPC code, but this
 * atari-specific code is completely reimplemented and
 * has some improvements for timeouts, which are
 * copyright by Ulrich Kuehn, 1993, 1994
 */

/*
 * File : svc.c
 *        service functions
 */

#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <macros.h>
#include "xdr.h"
#include "rpc.h"
#include "svc.h"




/*
 * This is the implementation of an UDP tranport handle.
 * (at the moment with UNIX-Domain sockets, but DGRAM)
 */

SVCXPRT *
svc_create(int sock, long sendsz, long recvsz)
{
	SVCXPRT *sx;
	struct sockaddr_in addr;
	long r;

	if (sock < 0)
	{
		if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
			return NULL;
	}

	r = sizeof(addr);
	r = getsockname(sock, (struct sockaddr*)&addr, &r);
	if (r != 0)
		return NULL;

	sx = malloc(sizeof(SVCXPRT));
	if (!sx)
	{
		return NULL;
	}

	sx->xp_sock = sock;
	sx->xp_port = ntohs(addr.sin_port);
	sx->xp_iosiz = ((max(sendsz, recvsz)+3) & (~3));
	sx->xp_p1 = malloc(sx->xp_iosiz);
	if (!sx->xp_p1)
		return NULL;

	xdr_init(&sx->xp_x, (char*)sx->xp_p1, sx->xp_iosiz, XDR_DECODE, NULL);
	sx->xp_xid = 0;
	return sx;
}

static int
rpc_check_direction(long dir, xdrs *x)
{
	if (x->length < 2*sizeof(long))
		return FALSE;

	return (((long *)x->current)[1] == dir);
}

bool_t
svc_recv(SVCXPRT *xprt, struct rpc_msg *msg)
{
	long len;

	xprt->xp_addrlen = sizeof(struct sockaddr_in);
	len = recvfrom(xprt->xp_sock, xprt->xp_p1, xprt->xp_iosiz, 0,
	                   (struct sockaddr*)&xprt->xp_raddr, &xprt->xp_addrlen);
	if (len < 0)
		return FALSE;

	if (!xdr_init(&xprt->xp_x, (char*)xprt->xp_p1, len, XDR_DECODE, NULL))
		return FALSE;

	/* ++kay: If xdr_rpc_msg() is called with an rpc message of the wrong
	 * direction, very bad things happen (crashes).
	 * So do the check before calling xdr_rpc_msg().
	 */
	if (!rpc_check_direction(CALL, &xprt->xp_x))
		return FALSE;

	/* HACK: to prevemt xdr_rpc_msg from decoding the function args */
	msg->cbody.xproc = NULL;
	if (!xdr_rpc_msg(&xprt->xp_x, msg))
		return FALSE;

	xprt->xp_xid = msg->xid;

	/* KLUDGE: we should copy here the verifier from the request message, but
	 *         at the moment we do not know to handle other auth_flavors than
	 *         AUTH_NULL and AUTH_UNIX.
	 */
	xprt->xp_verf = null_auth;
	return TRUE;
}


bool_t
svc_reply(SVCXPRT *xprt, struct rpc_msg *msg)
{
	long len, r;

	msg->xid = xprt->xp_xid;
	msg->mtype = REPLY;
	if (!xdr_init(&xprt->xp_x, (char*)xprt->xp_p1,
	                    xprt->xp_iosiz, XDR_ENCODE, NULL))
		return FALSE;
	if (!xdr_rpc_msg(&xprt->xp_x, msg))
		return FALSE;

	len = xdr_getpos(&xprt->xp_x);

	r = sendto(xprt->xp_sock, xprt->xp_p1, len, 0,
	                    (struct sockaddr*)&xprt->xp_raddr, xprt->xp_addrlen);
	if (r == len)
		return TRUE;

	return FALSE;
}


enum xprt_stat
svc_stat(SVCXPRT *xprt)
{
	return XPRT_IDLE;
}


bool_t
svc_getargs(SVCXPRT *xprt, xdrproc_t xdrarg, caddr_t argsp)
{
	return (*(xdrarg))(&xprt->xp_x, argsp);
}




/*
 * These functions are not specific to the actual implementation of a
 * given SVCXPRT. So at some point we might chose to implement different
 * kinds of transports.
 */

bool_t
svc_sendreply(SVCXPRT *xprt, xdrproc_t xdr_res, caddr_t loc)
{
	struct rpc_msg msg;

	msg.rbody.rb_stat = MSG_ACCEPTED;
	msg.rbody.rb_arpl.ar_stat = SUCCESS;
	msg.rbody.rb_arpl.ar_verf = xprt->xp_verf;
	msg.rbody.rb_arpl.ar_result.xproc = xdr_res;
	msg.rbody.rb_arpl.ar_result.data = loc;
	return svc_reply(xprt, &msg);
}

void
svcerr_noproc(SVCXPRT *xprt)
{
	struct rpc_msg msg;

	msg.rbody.rb_stat = MSG_ACCEPTED;
	msg.rbody.rb_arpl.ar_stat = PROC_UNAVAIL;
	msg.rbody.rb_arpl.ar_verf = xprt->xp_verf;
	svc_reply(xprt, &msg);
}

void
svcerr_decode(SVCXPRT *xprt)
{
	struct rpc_msg msg;

	msg.rbody.rb_stat = MSG_ACCEPTED;
	msg.rbody.rb_arpl.ar_stat = GARBAGE_ARGS;
	msg.rbody.rb_arpl.ar_verf = xprt->xp_verf;
	svc_reply(xprt, &msg);
}

void
svcerr_auth(SVCXPRT *xprt, enum auth_stat reason)
{
	struct rpc_msg msg;

	msg.rbody.rb_stat = MSG_DENIED;
	msg.rbody.rb_rrpl.rr_stat = AUTH_ERROR;
	msg.rbody.rb_rrpl.rr_astat = reason;
	svc_reply(xprt, &msg);
}

void
svcerr_weakauth(SVCXPRT *xprt)
{
	struct rpc_msg msg;

	msg.rbody.rb_stat = MSG_DENIED;
	msg.rbody.rb_rrpl.rr_stat = AUTH_ERROR;
	msg.rbody.rb_rrpl.rr_astat = AUTH_TOOWEAK;
	svc_reply(xprt, &msg);
}

void
svcerr_noprog(SVCXPRT *xprt)
{
	struct rpc_msg msg;

	msg.rbody.rb_stat = MSG_ACCEPTED;
	msg.rbody.rb_arpl.ar_stat = PROG_UNAVAIL;
	msg.rbody.rb_arpl.ar_verf = xprt->xp_verf;
	svc_reply(xprt, &msg);
}

void
svcerr_progvers(SVCXPRT *xprt, u_long low, u_long high)
{
	struct rpc_msg msg;

	msg.rbody.rb_stat = MSG_ACCEPTED;
	msg.rbody.rb_arpl.ar_stat = PROG_MISMATCH;
	msg.rbody.rb_arpl.ar_verf = xprt->xp_verf;
	msg.rbody.rb_arpl.ar_mis_info.low = low;
	msg.rbody.rb_arpl.ar_mis_info.high = high;
	svc_reply(xprt, &msg);
}

void
svcerr_rpcvers(SVCXPRT *xprt)
{
	struct rpc_msg msg;

	msg.rbody.rb_stat = MSG_DENIED;
	msg.rbody.rb_rrpl.rr_stat = RPC_MISMATCH;
	msg.rbody.rb_rrpl.rr_mis_info.low = RPC_VERSION;
	msg.rbody.rb_rrpl.rr_mis_info.high = RPC_VERSION;	
	svc_reply(xprt, &msg);
}	
