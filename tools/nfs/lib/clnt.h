
/*
 * File : clnt.h
 *        definitions for an RPC interface for clients
 */

#ifndef CLNT_H
#define CLNT_H

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include "xdr.h"
#include "rpc.h"


/* possible error codes */
enum clnt_stat
{
  RPC_SUCCESS = 0,          /* call successful */

  RPC_CANTENCODEARGS = 1,   /* cannot encode arguments */
  RPC_CANTDECODERES = 2,    /* cannot decode results */
  RPC_CANTSEND = 3,         /* cannot send request */
  RPC_CANTRECV = 4,         /* cannot receive a reply */
  RPC_TIMEDOUT = 5,         /* RPC call timed out */

  RPC_VERSMISMATCH = 6,     /* RPC version mismatch */
  RPC_AUTHERROR = 7,        /* authentification error */
  RPC_PROGUNAVAIL = 8,      /* remote program not available */
  RPC_PROGVERSMISMATCH = 9, /* program version mismatch */
  RPC_PROCUNAVAIL = 10,     /* requested procedure not available */
  RPC_CANTDECODEARGS = 11,  /* server cannot decode arguments */
  RPC_SYSTEMERROR = 12,     /* system error on server side */

  RPC_UNKNOWNHOST = 13,
  RPC_UNKNOWNPROTO = 17,

  RPC_PMAPFAILURE = 14,     /* failed to call portmapper service */
  RPC_PROGNOTREGISTERD = 15,  /* program not registred on portmapper */

  RPC_FAILED = 16           /* general error */
};


typedef struct client
{
  caddr_t cl_auth;   /* not used at the moment */
  int cl_sock;
  struct sockaddr_in cl_raddr;
  size_t cl_rlen;
  int cl_closeit;
  u_long cl_prog;
  u_long cl_vers;
  struct timeval cl_wait;
  struct timeval cl_total;
  xdrs cl_x;
  long cl_sendsz;
  char *cl_sendbuf;
  long cl_recvsz;
  char cl_recvbuf[4];   /* is allocated long enough */
} CLIENT;


CLIENT *clnt_create(struct sockaddr_in *raddr, u_long prog,
                                 u_long version, struct timeval wait,
                                 size_t sendsz, size_t recvsz, int *sockp);

enum clnt_stat clnt_call(CLIENT *cl, u_long proc,
                         xdrproc_t xargs, caddr_t argsp,
                         xdrproc_t xres, caddr_t resp, struct timeval total);

void clnt_abort(CLIENT *cl);

bool_t clnt_control(CLIENT *cl, int req, char *info);

void clnt_destroy(CLIENT *cl);


#define UDPMSGSIZE       8800
#define RPCSMALLMSGSIZE  400


#endif
