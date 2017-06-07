/*
 * $Id$
 * 
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution.
 * 
 * 
 * Copyright 2003 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * 
 * Author: Frank Naumann <fnaumann@freemint.de>
 * Started: 2003-12-13
 * 
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 */

# include "aranym.h"


# ifdef ARANYM

/* assembler routine */
int detect_native_features(void);


static unsigned long nf_get_id_instr = 0x73004e75UL;
static unsigned long nf_call_instr = 0x73014e75UL;

static struct nf_ops _nf_ops = { (void *)&nf_get_id_instr, (void *)&nf_call_instr }; 
static struct nf_ops *nf_ops = NULL; 

/*
 * nf_ops->call function reference.
 */
#if defined WITH_HOSTFS || defined WITH_ARANYMFS
long __CDECL (*nf_call)(long id, ...) = 0L;
#endif


struct nf_ops *
nf_init(void)
{
	if (detect_native_features())
	{
		nf_ops = &_nf_ops;
		return nf_ops;
	}
	
	return NULL;
}


char *
nf_name(void)
{
	static char buf[64] = "Unknown emulator";
	
	if (nf_ops)
	{
		static int done = 0;
		
		if (!done)
		{
			long nfid_name = nf_ops->get_id("NF_NAME");
			
			if (nfid_name)
				nf_ops->call(nfid_name, buf, sizeof(buf));
			
			done = 1;
		}
	}
        
	return buf;
}

int
nf_debug(const char *msg)
{
	if (nf_ops)
	{
		long nfid_stderr = nf_ops->get_id("NF_STDERR");
		
		if (nfid_stderr)
		{
			nf_ops->call(nfid_stderr, msg);
			return 1;
		}
	}
	
	return 0;
}

void
nf_shutdown(void)
{
	if (nf_ops)
	{
		long shutdown_id = nf_ops->get_id("NF_SHUTDOWN");
		
		if (shutdown_id)
        		nf_ops->call(shutdown_id);
	}
}

# endif
