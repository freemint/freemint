/*
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 */

/*
 * File : rpc.h
 *        definitions for remote procedure calls
 */

#ifndef RPC_H
#define RPC_H

#include "global.h"
#include "xdr.h"


#define RPC_VERSION   (2)   /* version of the RPC protocol */




/* the auth_flavor values */
enum auth_flavor
{
	AUTH_NULL  = 0,   /* no auth at all */
	AUTH_UNIX  = 1,   /* unix style auth (uid, gid) */
	AUTH_SHORT = 2,   /* short unix style auth */
	AUTH_DES   = 3    /* auth using DES */
};


#define MAX_AUTH_BYTES  400

typedef struct opaque_auth
{
	enum_t flavor;   /* enum auth_flavor flavor */
	opaque *data;
	ulong  len;
} opaque_auth;
 
extern opaque_auth null_auth;


/* RPC message types */
enum msg_type { CALL = 0, REPLY = 1 };

/* RPC reply status */
enum reply_stat { MSG_ACCEPTED = 0, MSG_DENIED = 1 };

/* RPC accepted message status */
enum accept_stat
{
	SUCCESS = 0,
	PROG_UNAVAIL = 1,
	PROG_MISMATCH = 2,
	PROC_UNAVAIL = 3,
	GARBAGE_ARGS = 4
};

/* RPC rejected message status */
enum reject_stat
{
	RPC_MISMATCH  = 0,  /* RPC version != 2 */
	AUTH_ERROR    = 1   /* authentification failed */
};

/* RPC authentification status */
enum auth_stat
{
	AUTH_OK           = 0,
	AUTH_BADCRED      = 1,  /* bad credentials, seal broken */
	AUTH_REJECTEDCRED = 2,  /* client must begin new session */
	AUTH_BADVERF      = 3,  /* bad verifier, seal broken */
	AUTH_REJECTEDVERF = 4,  /* verifier expired */
	AUTH_TOOWEAK      = 5   /* rejected for security reasons */
};


/* here are the host related representations of RPC structures */

struct call_body
{
	ulong rpcvers;
	ulong prog;
	ulong vers;
	ulong proc;
	struct opaque_auth cred;
	struct opaque_auth verf;
	/* procedure specific parameters start here */
	xdrproc_t xproc;   /* ignored if not set */
	caddr_t data;     /* but this point is filled in */
};


struct accepted_reply
{
	struct opaque_auth ar_verf;
	enum_t ar_stat;                /* enum accept_stat ar_stat */
	union
	{
		struct  /* stat == SUCCESS */
		{
			xdrproc_t xproc; /* function to convert representations, if not set, */
			caddr_t data;    /* results have to be encoded "by hand" */
		}	result;
		struct              /* stat == PROG_MISMATCH */
		{
			ulong low;   /* lowest/highest version number of remote program */
			ulong high;
		} mis_info;
		/* all other cases have void results */
	} ru;
};

#define ar_result    ru.result
#define ar_mis_info  ru.mis_info


struct rejected_reply
{
	enum_t rr_stat;           /* enum reject_stat rr_stat */
	union
	{
		struct                  /* rr_stat == RPC_MISMATCH */
		{
			ulong low;
			ulong high;
		} mis_info;
		enum_t astat;   /* enum auth_stat astat */
	} ru;
};

#define rr_mis_info  ru.mis_info
#define rr_astat     ru.astat


struct reply_body
{
	enum_t rb_stat;           /* enum reply_stat rb_stat */
	union
	{
		struct accepted_reply areply;
		struct rejected_reply rreply;
	} ru;
};

#define rb_arpl  ru.areply
#define rb_rrpl  ru.rreply


typedef struct rpc_msg
{
	ulong xid;            /* transaction id */
	enum_t mtype;         /* enum msg_type mtype */
	union
	{
		struct call_body cb;
		struct reply_body rb;
	} bd;
} rpc_msg;

#define cbody  bd.cb
#define rbody  bd.rb


long xdr_size_opaque_auth(opaque_auth *ap);
bool_t xdr_opaque_auth(xdrs *x, opaque_auth *ap);
long xdr_size_rpc_msg(rpc_msg *msg);
bool_t xdr_rpc_msg(xdrs *x, rpc_msg *msg);

#endif
