/*
 *	This file contains the stuff needed to send SIGURG to processes
 *	when urgent data arrives.
 *
 *	94/05/12, Kay Roemer.
 */

# include "tcpsig.h"

# include "mint/net.h"

# include "inet.h"


void
tcp_sendsig (struct tcb *tcb, short sig)
{
	if (tcb->data->sock == 0 || tcb->data->sock->pgrp == 0)
		return;
	
	ikill (tcb->data->sock->pgrp, sig);
}
