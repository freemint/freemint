/* File: auth_unix.h
 *       unix style rpc authentifier
 */

#ifndef AUTH_UNIX_H
#define AUTH_UNIX_H

#include <sys/types.h>
#include "xdr.h"
#include "rpc.h"


#define MAXMACHINENAME 255
#define NGRPS 16

typedef struct
{
	u_long au_time;
	char au_machname[MAXMACHINENAME+1];
	long au_uid;
	long au_gid;
	u_long au_len;
	long au_gids[NGRPS];
} auth_unix;


#endif
