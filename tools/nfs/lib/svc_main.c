/*
 * Copyright 1993, 1994 by Ulrich KÅhn. All rights reserved.
 *
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 */

#include <osbind.h>
#include <mintbind.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/types.h>
#include "xdr.h"
#include "rpc.h"
#include "svc.h"
#include "pmap.h"


typedef struct svc_rec
{
	struct svc_rec *sr_next;
	SVCXPRT *sr_xprt;         /* the transport handle */
	u_long sr_prog;           /* program number */
	u_long sr_vers;           /* program version */
	int sr_proto;
	svcproc_t sr_dispatch;    /* dispatch function */
	svctimeproc_t sr_timeout; /* optional timeout function */
} SVC_REC;


#define RQ_CREDSIZE  400


static SVC_REC *svc_services = NULL;
static long svc_timeoutval = 0;

#define NFILES  32
static long svc_fdset = 0;
static SVCXPRT *svc_xprts[NFILES];


extern enum auth_stat authenticate(struct svc_req *req, struct rpc_msg *msg);


bool_t
svc_register(SVCXPRT *xprt, u_long prog, u_long vers,
                            svcproc_t dispatch, int proto)
{
	SVC_REC *sr;

	sr = malloc(sizeof(SVC_REC));
	if (!sr)
		return FALSE;

	sr->sr_xprt = xprt;
	sr->sr_prog = prog;
	sr->sr_vers = vers;
	sr->sr_proto = proto;
	sr->sr_dispatch = dispatch;
	sr->sr_timeout = NULL;
	sr->sr_next = svc_services;
	svc_services = sr;

	if (proto)
	{
		/* first cancel a possible old registration */
		pmap_unset(prog, vers);
		pmap_set(prog, vers, proto, xprt->xp_port);
	}

	return TRUE;
}


void
svc_unregister(u_long prog, u_long vers)
{
	SVC_REC *ps, **pps;

	ps = svc_services;
	pps = &svc_services;
	while (ps)
	{
		if ((ps->sr_prog == prog) && (ps->sr_vers == vers))
		{
			*pps = ps->sr_next;
			free(ps);
		}
		pps = &ps->sr_next;
		ps = ps->sr_next;
	}
	pmap_unset(prog, vers);
}


long
svc_timeout(SVCXPRT *xprt, long timeout, svctimeproc_t fn)
{
	SVC_REC *sr;
	long r;

	for (sr = svc_services;  sr;  sr = sr->sr_next)
	{
		if (sr->sr_xprt == xprt)
		{
			sr->sr_timeout = fn;
			r = svc_timeoutval;
			svc_timeoutval = timeout;
			return r;
		}
	}
	return 0;
}


void
xprt_register(SVCXPRT *xprt)
{
	if ((xprt->xp_sock < 0) || (xprt->xp_sock >= NFILES))
		return;

	svc_fdset |= (1L << xprt->xp_sock);
	svc_xprts[xprt->xp_sock] = xprt;
}


void
xprt_unregister(SVCXPRT *xprt)
{
	if ((xprt->xp_sock < 0) || (xprt->xp_sock >= NFILES))
		return;

	svc_fdset &= ~(1L << xprt->xp_sock);
	svc_xprts[xprt->xp_sock] = NULL;
}



/*
 * This is the request dispatcher which takes as argument a set of
 * file descriptors that are ready for reading. It reads each of them out
 * and processes the request.
 */
void
svc_getreqfd(long fdset)
{
	SVC_REC *sr;
	SVCXPRT *xprt;
	int s;
	long mask;
	struct rpc_msg msg;
	struct svc_req req;
	u_long p_min, p_max;
	char cred_raw[2*MAX_AUTH_BYTES+RQ_CREDSIZE];
	opaque_auth cred_auth, verf_auth;
	enum auth_stat auth_res;

	for (s = 0, mask = 1;  s < NFILES;  s++, mask <<= 1)
	{
		if (fdset & mask)
		{
			/* here we know that the socket s is ready for reading, so read it
			 * out until there is nothing more.
			 */
			xprt = svc_xprts[s];
			if (!xprt)
				continue;

			do {
				cred_auth.data = &cred_raw[0];
				verf_auth.data = &cred_raw[MAX_AUTH_BYTES];
				req.rq_clntcred = &cred_raw[2*MAX_AUTH_BYTES];

				msg.cbody.cred = cred_auth;
				msg.cbody.verf = verf_auth;
				if (!svc_recv(xprt, &msg))
					continue;

				req.rq_prog = msg.cbody.prog;
				req.rq_vers = msg.cbody.vers;
				req.rq_proc = msg.cbody.proc;
				req.rq_cred = msg.cbody.cred;
				req.rq_xprt = xprt;

				auth_res = authenticate(&req, &msg);
				if (auth_res != AUTH_OK)
				{
					svcerr_auth(xprt, auth_res);
					continue;
				}

				/* search for the requested program number */
				p_min = p_max = 1;   /* dummy values to make th compiler happy */
				for (sr = svc_services;  sr;  sr = sr->sr_next)
					if (sr->sr_prog == req.rq_prog)
					{
						p_min = sr->sr_vers;
						p_max = sr->sr_vers;
						break;
					}

				if (!sr)
				{
					svcerr_noprog(xprt);
					continue;
				}

				/* now search for the right program number, but also for the
				 * requested program version
				 */
				for ( ;  sr;  sr = sr->sr_next)
					if (sr->sr_prog == req.rq_prog)
					{
						if (sr->sr_vers == req.rq_vers)
							break;
					 	if (sr->sr_vers < p_min)
					 		p_min = sr->sr_vers;
					 	if (sr->sr_vers > p_max)
					 		p_max = sr->sr_vers;
					}

				if (!sr)
				{
					svcerr_progvers(xprt, p_min, p_max);
					continue;
				}

				(*sr->sr_dispatch)(&req, xprt);
			} while (svc_stat(xprt) == XPRT_MOREREQS);
		}
	}
}



/*
 * The server idle loop. It waits for a timeout (if set) and one or more
 * transport handles to become ready for reading. Then these handles are
 * read and the requests are processed.
 */
void
svc_run()
{
	long r, rdset;
	SVC_REC *sr;

	for (;;)
	{
		rdset = svc_fdset;
		r = Fselect(svc_timeoutval, &rdset, 0, 0);
		if (r == 0)
		{
			for (sr = svc_services;  sr;  sr = sr->sr_next)
			{
				if (sr->sr_timeout)
					(*sr->sr_timeout)(svc_timeoutval);
			}
		}
		else
		{
			/* KLUDGE: as select does not return EINTR if there was a signal, we
			 *         have to check it here.
			 */
			if (!rdset)
				continue;

			svc_getreqfd(rdset);
		}
	}
}
