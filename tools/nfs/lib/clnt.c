/*
 * Copyright 1993, 1994 by Ulrich KÅhn. All rights reserved.
 *
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 */

/*
 * File : clnt.c
 *        Implementation of RPC client fucntions
 */

#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <osbind.h>
#include <mintbind.h>
#include "xdr.h"
#include "rpc.h"
#include "clnt.h"
#include "pmap.h"

#include <string.h>



CLIENT *
clnt_create(struct sockaddr_in *raddr, u_long prog, u_long vers,
              struct timeval wait, size_t sendsz, size_t recvsz, int *sockp)
{
	int s, close_it;
	long r;
	CLIENT *cl;


	if (raddr->sin_port == 0)
	{
		raddr->sin_port = pmap_getport(raddr, prog, vers, IPPROTO_UDP);
	}
	if (raddr->sin_port == 0)
		return NULL;

	if (sockp)
		s = *sockp;
	else
		s = -1;
	if (s < 0)
	{
		struct sockaddr_in sin;

		s = socket(PF_INET, SOCK_DGRAM, 0);
		if (s < 0)
			return NULL;

		/* bind the socket to some random port */
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = htonl(INADDR_ANY);
		sin.sin_port = htons(0);

		r = bind(s, (struct sockaddr*)&sin, sizeof(sin));
		if (r < 0)
		{
			(void) Fclose(s);
			return NULL;
		}

		close_it = TRUE;
		if (sockp)
			*sockp = s;
	}
	else
		close_it = FALSE;

	/* BUG: bad hack! we do not have this value! */
	if (recvsz < 4096)
		recvsz = 4096;

	r = setsockopt(s, SOL_SOCKET, SO_RCVBUF, (void*)&recvsz, sizeof(size_t));
	if (r < 0)
		return NULL;

	cl = malloc(sizeof(CLIENT) + sendsz + recvsz);
	if (cl == NULL)
		return NULL;

	cl->cl_auth = NULL;   /* BUG! */
	cl->cl_sock = s;
	cl->cl_raddr = *raddr;
	cl->cl_rlen = sizeof(struct sockaddr_in);
	cl->cl_closeit = close_it;
	cl->cl_prog = prog;
	cl->cl_vers = vers;
	cl->cl_wait = wait;

	/* these are default values */
	cl->cl_total.tv_sec = -1;
	cl->cl_total.tv_usec = 0;

	/* set the buffers */
	cl->cl_sendsz = sendsz;
	cl->cl_recvsz = recvsz;
	cl->cl_sendbuf = &cl->cl_recvbuf[0] + recvsz;
	return cl;
}


void
clnt_destroy(CLIENT *cl)
{
	if (cl->cl_closeit)
		(void) Fclose(cl->cl_sock);
	free(cl);
}


enum clnt_stat
clnt_call(CLIENT *cl, u_long proc, xdrproc_t xargs, caddr_t argsp,
                         xdrproc_t xres, caddr_t resp, struct timeval total)
{
	static u_long clnt_xid = 0;  /* must really be static */
	u_long our_xid;
	struct rpc_msg msg;
	long slen, rlen, r, fdset;
	long timeout, totalwait, waited;
	struct sockaddr_in from;
	long fromlen;

	if (!cl)
		return RPC_FAILED;

	our_xid = clnt_xid++;
	msg.xid = our_xid;
	msg.mtype = CALL;
	msg.cbody.rpcvers = RPC_VERSION;
	msg.cbody.prog = cl->cl_prog;
	msg.cbody.vers = cl->cl_vers;
	msg.cbody.proc = proc;

	/* BUG: we should use here the cl_auth member in the CLIENT structure */
	msg.cbody.cred = null_auth;
	msg.cbody.verf = null_auth;
	msg.cbody.xproc = xargs;
	msg.cbody.data = argsp;

	if (!xdr_init(&cl->cl_x, cl->cl_sendbuf, cl->cl_sendsz, XDR_ENCODE, NULL))
		return RPC_CANTENCODEARGS;

	if (!xdr_rpc_msg(&cl->cl_x, &msg))
		return RPC_CANTENCODEARGS;

	/* Now the send buffer contais the RPC message. We have to do the
	 * send/resend/get reply stuff here.
	 */
	slen = xdr_getpos(&cl->cl_x);
	rlen = 0;
	if (cl->cl_total.tv_sec == -1)  /* if set, use default value */
		totalwait = total.tv_sec * 1000;
	else
		totalwait = cl->cl_total.tv_sec * 1000;
	timeout = cl->cl_wait.tv_sec * 1000;
	waited = 0;

	/* preset the result decoder function in the message header */
	msg.rbody.rb_arpl.ar_result.xproc = xres;
	msg.rbody.rb_arpl.ar_result.data = resp;

resend:
	r = sendto(cl->cl_sock, (void*)cl->cl_sendbuf, slen, 0,
	                    (struct sockaddr*)&cl->cl_raddr, (size_t)cl->cl_rlen);
	if (r != slen)
		return RPC_CANTSEND;

	for (;;)
	{
		fdset = (1L << cl->cl_sock);
		r = Fselect(timeout, &fdset, 0, 0);
		if (r <= 0)
		{
			/* here we should check for EINTR and continue; for any other error, we
			 * should return with RPC_CANTRECV.
			 */
			waited += timeout;
			if (waited > totalwait)
				return RPC_TIMEDOUT;
			goto resend;
		}
		r = recvfrom(cl->cl_sock, cl->cl_recvbuf, cl->cl_recvsz, 0,
		                          (struct sockaddr*)&from, (size_t*)&fromlen);
		if (r < 0)
			return RPC_CANTRECV;

		/* check the xid of the reply message */
		rlen = r;
		if ( (*(long*)&cl->cl_sendbuf[0]) == (*(long*)&cl->cl_recvbuf[0]) )
			break;   /* here we assume to have the correct reply message */
	}
	if (rlen == 0)  /* no reply */
		return RPC_TIMEDOUT;

	if (!xdr_init(&cl->cl_x, cl->cl_recvbuf, rlen, XDR_DECODE, NULL))
		return RPC_CANTDECODERES;
	if (!xdr_rpc_msg(&cl->cl_x, &msg))
		return RPC_CANTDECODERES;

	/* now check the reply status in the message header */
	if (MSG_ACCEPTED != msg.rbody.rb_stat)
	{
		/* request was not accepted */
		if (RPC_MISMATCH == msg.rbody.rb_rrpl.rr_stat)
			return RPC_VERSMISMATCH;
		if (AUTH_ERROR == msg.rbody.rb_rrpl.rr_stat)
			return RPC_AUTHERROR;
		return RPC_FAILED;
	}

	switch (msg.rbody.rb_arpl.ar_stat)
	{
		case SUCCESS:
			return RPC_SUCCESS;
		case PROG_UNAVAIL:
			return RPC_PROGUNAVAIL;
		case PROG_MISMATCH:
			return RPC_PROGVERSMISMATCH;
		case PROC_UNAVAIL:
			return RPC_PROCUNAVAIL;
		default:
			return RPC_FAILED;
	}
	return RPC_FAILED;
}


void
clnt_abort(CLIENT *cl)
{
	/* do nothing */
}


bool_t
clnt_control(CLIENT *cl, int req, char *info)
{
	return FALSE;
}
