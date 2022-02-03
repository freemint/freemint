/*
 * Adopted to Mint-Net 1994, Kay Roemer
 *
 * Modified to support Pure-C, Thorsten Otto.
 */

/*
 * Copyright (c) 1985, 1989 Regents of the University of California.
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

/*
 * Send query to name server and wait for reply.
 */

#include "stsocket.h"
#include "pcerrno.h"


static int s = -1;			/* socket used for communications */
static struct sockaddr no_addr;


int res_send(const uint8_t *buf, int buflen, uint8_t *answer, int anslen)
{
	int n;
	int try, v_circuit, resplen, ns;
	int gotsomewhere = 0;
	int connected = 0;
	int connreset = 0;
	uint16_t id, len;
	uint8_t *cp;
	fd_set dsmask;
	struct timeval timeout;
	const HEADER *hp = (const HEADER *) buf;
	HEADER *anhp = (HEADER *) answer;
	struct iovec iov[2];
	int terrno = ETIMEDOUT;
	char junk[512];

	if ((_res.options & RES_INIT) == 0)
		if (res_init() < 0)
		{
			return -1;
		}
	v_circuit = (_res.options & RES_USEVC) || buflen > NS_PACKETSZ;
	id = hp->id;
	/*
	 * Send request, RETRY times, or until successful
	 */
	for (try = 0; try < _res.retry; try++)
	{
		for (ns = 0; ns < _res.nscount; ns++)
		{
		  usevc:
			if (v_circuit)
			{
				int truncated = 0;

				/*
				 * Use virtual circuit;
				 * at most one attempt per server.
				 */
				try = _res.retry;
				if (s < 0)
				{
					s = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
					if (s < 0)
					{
						terrno = errno;
						continue;
					}
					if (connect(s, (struct sockaddr *) &(_res.nsaddr_list[ns]), (socklen_t)sizeof(struct sockaddr)) < 0)
					{
						terrno = errno;
						close(s);
						s = -1;
						continue;
					}
				}
				/*
				 * Send length & message
				 */
				len = htons((uint16_t) buflen);
				iov[0].iov_base = &len;
				iov[0].iov_len = sizeof(len);
				iov[1].iov_base = (void *)buf;
				iov[1].iov_len = buflen;
				{
					struct msghdr msg;

					msg.msg_name = 0;
					msg.msg_namelen = 0;
					msg.msg_iov = iov;
					msg.msg_iovlen = 2;
					msg.msg_control = 0;
					msg.msg_controllen = 0;

					if ((size_t)sendmsg(s, &msg, 0) != sizeof(len) + buflen)
					{
						terrno = errno;
						close(s);
						s = -1;
						continue;
					}
				}
				/*
				 * Receive length & response
				 */
				cp = answer;
				len = sizeof(uint16_t);
				while (len != 0 && (n = (int)read(s, cp, len)) > 0)
				{
					cp += n;
					len -= n;
				}
				if (n <= 0)
				{
					terrno = errno;
					close(s);
					s = -1;
					/*
					 * A long running process might get its TCP
					 * connection reset if the remote server was
					 * restarted.  Requery the server instead of
					 * trying a new one.  When there is only one
					 * server, this means that a query might work
					 * instead of failing.  We only allow one reset
					 * per query to prevent looping.
					 */
					if (terrno == ECONNRESET && !connreset)
					{
						connreset = 1;
						ns--;
					}
					continue;
				}
				cp = answer;
				if ((resplen = ntohs(*(uint16_t *) cp)) > anslen)
				{
					len = anslen;
					truncated = 1;
				} else
				{
					len = resplen;
				}
				while (len != 0 && (n = (int)read(s, cp, len)) > 0)
				{
					cp += n;
					len -= n;
				}
				if (n <= 0)
				{
					terrno = errno;
					close(s);
					s = -1;
					continue;
				}
				if (truncated)
				{
					/*
					 * Flush rest of answer
					 * so connection stays in synch.
					 */
					anhp->tc = 1;
					len = resplen - anslen;
					while (len != 0)
					{
						n = len > sizeof(junk) ? (int)sizeof(junk) : len;
						if ((n = (int)read(s, junk, n)) > 0)
							len -= n;
						else
							break;
					}
				}
			} else
			{
				/*
				 * Use datagrams.
				 */
				if (s < 0)
				{
					s = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
					if (s < 0)
					{
						terrno = errno;
						continue;
					}
				}
				/*
				 * I'm tired of answering this question, so:
				 * On a 4.3BSD+ machine (client and server,
				 * actually), sending to a nameserver datagram
				 * port with no nameserver will cause an
				 * ICMP port unreachable message to be returned.
				 * If our datagram socket is "connected" to the
				 * server, we get an ECONNREFUSED error on the next
				 * socket operation, and select returns if the
				 * error message is received.  We can thus detect
				 * the absence of a nameserver without timing out.
				 * If we have sent queries to at least two servers,
				 * however, we don't want to remain connected,
				 * as we wish to receive answers from the first
				 * server to respond.
				 */
				if (_res.nscount == 1 || (try == 0 && ns == 0))
				{
					/*
					 * Don't use connect if we might
					 * still receive a response
					 * from another server.
					 */
					if (connected == 0)
					{
						if (connect(s, (struct sockaddr *) &_res.nsaddr_list[ns], (socklen_t)sizeof(struct sockaddr)) < 0)
						{
							continue;
						}
						connected = 1;
					}
					if (send(s, buf, buflen, 0) != buflen)
					{
						continue;
					}
				} else
				{
					/*
					 * Disconnect if we want to listen
					 * for responses from more than one server.
					 */
					if (connected)
					{
						connect(s, &no_addr, (socklen_t)sizeof(no_addr));
						connected = 0;
					}
					if (sendto(s, buf, buflen, 0,
							   (struct sockaddr *) &_res.nsaddr_list[ns], (socklen_t)sizeof(struct sockaddr)) != buflen)
					{
						continue;
					}
				}

				/*
				 * Wait for reply
				 */
				timeout.tv_sec = _res.retrans << try;
				if (try > 0)
					timeout.tv_sec /= _res.nscount;
				if (timeout.tv_sec <= 0)
					timeout.tv_sec = 1;
				timeout.tv_usec = 0;
			  wait:
				FD_ZERO(&dsmask);
				FD_SET(s, &dsmask);
				n = select(s + 1, &dsmask, NULL, NULL, &timeout);
				if (n < 0)
				{
					continue;
				}
				if (n == 0)
				{
					/*
					 * timeout
					 */
					gotsomewhere = 1;
					continue;
				}
				if ((resplen = recv(s, answer, anslen, 0)) <= 0)
				{
					continue;
				}
				gotsomewhere = 1;
				if (id != anhp->id)
				{
					/*
					 * response from old query, ignore it
					 */
					goto wait;
				}
				if (!(_res.options & RES_IGNTC) && anhp->tc)
				{
					/*
					 * get rest of answer;
					 * use TCP with same server.
					 */
					close(s);
					s = -1;
					v_circuit = 1;
					goto usevc;
				}
			}
			/*
			 * If using virtual circuits, we assume that the first server
			 * is preferred * over the rest (i.e. it is on the local
			 * machine) and only keep that one open.
			 * If we have temporarily opened a virtual circuit,
			 * or if we haven't been asked to keep a socket open,
			 * close the socket.
			 */
			if ((v_circuit && ((_res.options & RES_USEVC) == 0 || ns != 0)) || (_res.options & RES_STAYOPEN) == 0)
			{
				close(s);
				s = -1;
			}
			return resplen;
		}
	}
	if (s >= 0)
	{
		close(s);
		s = -1;
	}
	if (v_circuit == 0)
	{
		if (gotsomewhere == 0)
		{
			__set_errno(ECONNREFUSED);	/* no nameservers found */
		} else
		{
			__set_errno(ETIMEDOUT);		/* no answer obtained */
		}
	} else
	{
		__set_errno(terrno);
	}
	return -1;
}


/*
 * This routine is for closing the socket if a virtual circuit is used and
 * the program wants to close it.  This provides support for endhostent()
 * which expects to close the socket.
 *
 * This routine is not expected to be user visible.
 */
void _res_close(void)
{
	if (s != -1)
	{
		close(s);
		s = -1;
	}
}
