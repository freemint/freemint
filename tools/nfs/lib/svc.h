/*
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 */

/*
 * File : svc.h
 *        some service functions for daemons
 */

#ifndef SVC_H
#define SVC_H


#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include "rpc.h"
#include "xdr.h"


enum xprt_stat
{
	XPRT_DIED,
	XPRT_MOREREQS,
	XPRT_IDLE
};


typedef struct
{
	int xp_sock;   /* the transport socket handle */
	unsigned short xp_port;   /* the port number */
	size_t xp_addrlen;
	struct sockaddr_in xp_raddr;
	struct opaque_auth xp_verf;

	/* all this is private data. Do not touch */
	caddr_t *xp_p1;   /* send/recv buffer */
	long xp_iosiz;  /* size of the send/recv buffer */
	long xp_xid;
	xdrs xp_x;
} SVCXPRT;


/*
 * get the address of the caller
 */
#define svc_getcaller(x)  (&(x)->xp_raddr)


/*
 * Operations defined on SVCXPRT handles
 */

 /* create a svc handle and allocate buffers etc. */
SVCXPRT *svc_create(int sock, long sendsz, long recvsz);

/* receive next message from socket, decode header etc, set pointer to args */
bool_t svc_recv(SVCXPRT *xprt, struct rpc_msg *msg);

/* send a reply message */
bool_t svc_reply(SVCXPRT *xprt, struct rpc_msg *msg);

enum xprt_stat svc_stat(SVCXPRT *xprt);

/* extract the arguments from the buffer */
bool_t svc_getargs(SVCXPRT *xprt, xdrproc_t xarg, caddr_t argsp);

/* send a reply message from host data */
bool_t svc_sendreply(SVCXPRT *xprt, xdrproc_t xdr_res, caddr_t loc);

/* send some error messages */
void svcerr_noproc(SVCXPRT *xprt);
void svcerr_decode(SVCXPRT *xprt);
void svcerr_auth(SVCXPRT *xprt, enum auth_stat reason);
void svcerr_weakauth(SVCXPRT *xprt);
void svcerr_noprog(SVCXPRT *xprt);
void svcerr_progvers(SVCXPRT *xprt, u_long low, u_long high);
void svcerr_rpcvers(SVCXPRT *xprt);


struct svc_req
{
	u_long rq_prog;   /* requested program number */
	u_long rq_vers;   /* program version */
	u_long rq_proc;   /* requested procedure */
	struct opaque_auth rq_cred;
	caddr_t rq_clntcred;
	SVCXPRT *rq_xprt;
};

typedef void (*svcproc_t)(struct svc_req *, SVCXPRT *);
typedef void (*svctimeproc_t)(long);


bool_t svc_register(SVCXPRT *xprt, u_long prog, u_long vers,
                                 svcproc_t dispatch, int proto);

void svc_unregister(u_long prog, u_long vers);

void xprt_register(SVCXPRT *xprt);
void xprt_unregister(SVCXPRT *xprt);

long svc_timeout(SVCXPRT *xprt, long timeout, svctimeproc_t fn);


/* start the server, never returns */
void svc_run();


#endif
