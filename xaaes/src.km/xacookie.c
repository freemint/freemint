#include "global.h"
#include "debug.h"
#include "xacookie.h"
#include "mint/ssystem.h"

int install_cookie( void **addr, void *buff, size_t sz, long magic )
{
	char mstr[5];

	memcpy( mstr, (char*)&magic, 4 );
	mstr[4] = 0;

	if ( *addr || (*addr = (void*)m_xalloc(sz, 0x40 | 0x8 | 0x3 ) ) )
	{
		long r;

		memcpy(*addr, buff, sz );
		if ( (r=s_system(S_SETCOOKIE, magic, (long)*addr)) != 0)
		{
			m_free(*addr);
			*addr= NULL;
			BLOG((false, "Installing '%s' cookie failed (%ld)!", mstr, r));
			return 1;
		}
		else
		{
			BLOG((false, "Installed '%s' cookie in readable memory at %lx", mstr, (long)addr));
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

	memcpy( mstr, (char*)&magic, 4 );
	mstr[4] = 0;

	if( (r = s_system(S_DELCOOKIE, magic, 0L) == 0) )
	{
		BLOG((false, "Deleted '%s' cookie at %lx (free=%d)", mstr, (long)addr, free));
		if( *addr && free )
		{
			m_free(*addr);
			*addr = NULL;
		}
	}
	else
		BLOG((false, "Deletetion of '%s' cookie at %lx failed (r=%ld,free=%d)!", mstr, (long)addr, r, free));
}

