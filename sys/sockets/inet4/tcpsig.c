/*
 *	This file contains the stuff needed to send SIGURG to processes
 *	when urgent data arrives. (Currently this is a separate process).
 *
 *	94/05/12, Kay Roemer.
 */

# include "tcpsig.h"

# include "sockutil.h"
# include "util.h"

# include <mint/basepage.h>
# include <mint/file.h>

# include <mintbind.h>


# define STKSIZE		1024L

# define NSIGREQ		32
struct sigreq
{
	short	signum;
	short	pgrp;
};

static struct sigreq	allreq[NSIGREQ];
static char		*pname = "u:\\pipe\\tcpd";
static short		glfd;


void
tcp_sendsig (struct tcb *tcb, short sig)
{
	short i;
	char buf;
	
	if (tcb->data->sock == 0 || tcb->data->sock->pgrp == 0)
		return;
	
	if (ikill && iwake)
	{
		ikill (tcb->data->sock->pgrp, sig);
		return;
	}
	
	for (i = 0; i < NSIGREQ; ++i)
	{
		if (allreq[i].pgrp == 0)
		{
			allreq[i].pgrp = tcb->data->sock->pgrp;
			allreq[i].signum = sig;
			f_write (glfd, 1L, &buf);
			return;
		}
	}
	
	ALERT (("tcp_sendsig: no signal slot free"));
}

static int
tcpd (long bp)
{
	static char buf;
	short fd, i;
	
	setstack (bp + STKSIZE);
	Psigblock (-1L);
	fd = Fopen (pname, O_RDONLY);
	
	while (1)
	{
		while (Fread (fd, 1L, &buf) != 1)
			;
		
		for (i = 0; i < NSIGREQ; ++i)
		{
			if (allreq[i].pgrp != 0)
			{
				(void) Pkill (allreq[i].pgrp, allreq[i].signum);
				allreq[i].pgrp = 0;
			}
		}
	}
}

void
tcp_siginit (void)
{
	BASEPAGE *b;
	
	if (ikill && iwake)
		return;
	
	glfd = f_open (pname, O_NDELAY|O_WRONLY|O_CREAT|O_GLOBAL);
	if (glfd < 100)
		glfd += 100;
	
	f_chmod (pname, 0600);
	
	b = (BASEPAGE *) p_exec (PE_CBASEPAGE, 0L, "", 0L);
	m_shrink (0, (long) b, STKSIZE + 256L);
	
	b->p_tbase = (long) tcpd;
	b->p_hitpa = (long) b + STKSIZE + 256L;
	
	p_exec (104, "tcpd", b, 0L);
}
