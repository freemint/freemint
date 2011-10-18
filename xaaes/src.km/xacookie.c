#include "global.h"
#include "debug.h"
#include "xacookie.h"
#include "mint/ssystem.h"
#include "mint/proc.h"
#include "mint/mem.h"

int install_cookie( void **addr, void *buff, size_t sz, long magic, bool readonly )
{
	char mstr[5];
	short rw;

	if( readonly )
		rw = F_PROT_PR;
	else
		rw = F_PROT_G;

	if( !addr )
	{
		BLOG((0,"%s:install_cookie:ERROR:addr=0", get_curproc()->name));
		return 3;
	}

	memcpy( mstr, (char*)&magic, 4 );
	mstr[4] = 0;

	if ( *addr || (*addr = (void*)m_xalloc(sz, rw | 0x3 ) ) )
	{
		long r;

		memcpy(*addr, buff, sz );
		if ( (r=s_system(S_SETCOOKIE, magic, (long)*addr)) != 0)
		{
			BLOG((false, "Installing '%s' cookie failed (%ld,addr=%lx)!", mstr, r, *addr));
			m_free(*addr);
			*addr= NULL;
			return 1;
		}
		else
		{
			BLOG((false, "%s:Installed '%s' cookie in readable memory at %lx", get_curproc()->name, mstr, *(long*)addr));
			return 0;
		}
	}
	else
	{
		BLOG((false, "Could not get memory for '%s' cookie! (Mxalloc() failed)", mstr));
		return 2;
	}
}

void delete_cookie( void **addr, long magic, int free)
{
	long r;
	char mstr[5];

	if( !addr )
	{
		BLOG((0,"delete_cookie:ERROR:addr=0"));
		return;
	}
	memcpy( mstr, (char*)&magic, 4 );
	mstr[4] = 0;

	if( (r = s_system(S_DELCOOKIE, magic, 0L) == 0) )
	{
		BLOG((false, "Deleted '%s' cookie at %lx (free=%d)", mstr, *(long*)addr, free));
		if( *addr && free )
		{
			m_free(*addr);
			*addr = NULL;
		}
	}
	else
		BLOG((false, "Deletetion of '%s' cookie at %lx failed (r=%ld,free=%d)!", mstr, *(long*)addr, r, free));
}

