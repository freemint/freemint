/*
 * Copyright (c) 1985, 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <stdlib.h>
#ifdef __GNUC__
#include <unistd.h>
#else
int gethostname (char *__name, size_t __len);
#endif
#include <sys/socket.h>


/*
 * define MAGIC_ONLY to omit calls to Fsocket() etc.
 * This saves some overhead in the library when
 * used on MagiC (because those functions are not
 * supported by MagiCNet), but creates some overhead
 * in the MiNT kernel when it has to emulate the
 * old socket calls.
 */
/* #define MAGIC_ONLY 1 */

/*
 * Define UNIX_DOMAIN_SUPPORT to include suport for
 * unix domain (AF_UNIX) sockets
 */
#define UNIX_DOMAIN_SUPPORT 1


#if (defined(__MINT__) || defined(__PUREC__)) && !defined(__atarist__)
#define __atarist__ 1
#endif

#include <netdb.h>
#include <time.h>
#include <fcntl.h>
#ifdef __PUREC__
#include <tos.h>
#else
#include <mint/mintbind.h>
#endif
#undef ENOSYS
#define ENOSYS 32

#include <sys/param.h>
#include <resolv.h>
#include <netinet/in.h>
#include <net/if.h>						/* for struct ifconf */
#include <arpa/inet.h>
#include <sys/socket.h>
#include <arpa/nameser.h>
#include <sys/ioctl.h>
#ifdef UNIX_DOMAIN_SUPPORT
#include <sys/un.h>
#endif

#undef _PATH_HOSTS
#define _PATH_HOSTS     "U:\\etc\\hosts"
#undef _PATH_HOSTCONF
#define _PATH_HOSTCONF  "U:\\etc\\host.conf"
#undef _PATH_RESCONF
#define _PATH_RESCONF   "U:\\etc\\resolv.conf"


#if NS_PACKETSZ > 1024
#define	MAXPACKET	NS_PACKETSZ
#else
#define	MAXPACKET	1024
#endif

#ifdef __PUREC__
#define strcasecmp(a,b)		stricmp(a,b)
#define strncasecmp(a,b,c)	strnicmp(a,b,c)
#endif

#ifndef __res_state_defined
# define __res_state_defined
struct state {
	int	retrans;	 	/* retransmition time interval */
	int	retry;			/* number of times to retransmit */
	unsigned long options;		/* option flags - see below. */
	int	nscount;		/* number of name servers */
	struct sockaddr_in nsaddr_list[MAXNS];	/* address of name server */
#define	nsaddr nsaddr_list[0]		/* for backward compatibility */
	uint16_t id;		/* current packet id */
	char defdname[NS_MAXDNAME];	/* default domain */
	char *dnsrch[MAXDNSRCH+1];	/* components of domain to search */
};
#endif

#undef __set_errno
#define __set_errno(e) (errno = (e))

#ifndef howmany
# define howmany(x, y)	(((x)+((y)-1))/(y))
#endif

extern int h_errno;
extern struct state _res;
extern short __libc_newsockets;


#ifdef UNIX_DOMAIN_SUPPORT
int _unx2dos(const char *src, char *dst, size_t len);
#if 0
int _dos2unx(const char *src, char *dst, size_t len);
#endif
int _sncpy(char *dst, const char *src, size_t len);
#endif


int __dn_skipname(const uint8_t *comp_dn, const uint8_t *eom);
int dn_expand(const uint8_t *msg, const uint8_t *eomorig, const uint8_t *comp_dn, char *exp_dn, int length);
int dn_comp(const char *exp_dn, uint8_t *comp_dn, int length, uint8_t **dnptrs, uint8_t **lastdnptr);

int res_search(const char *name, int class, int type, uint8_t *answer, int anslen);
int res_query(const char *name, int class, int type, uint8_t *answer, int anslen);
int res_mkquery(int op, const char *dname, int class, int type, char *data, int datalen, struct rrec *newrr, uint8_t *buf, int buflen);
const char *__hostalias(const char *name);
int res_init(void);
int res_send(const uint8_t *buf, int buflen, uint8_t *answer, int anslen);
int res_querydomain(const char *name, const char *domain, int class, int type, uint8_t *answer, int anslen);
void _res_close(void);
