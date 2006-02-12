/*
 * $Id$
 * 
 * HypView - (c)      - 2006 Philipp Donze
 *               2006 -      Philipp Donze & Odd Skancke
 *
 * A replacement hypertext viewer
 *
 * This file is part of HypView.
 *
 * HypView is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * HypView is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HypView; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef __GNUC__
	#include <mintbind.h>
	#include <mint/sysvars.h>
	#include <mint/ssystem.h>
#else
	#include <tos.h>
	#include <sysvars.h>
#endif
#include "include/cookie.h"

#ifndef NDIAG
#define NDIAG(x) 
#endif
#ifndef DIAG
#define DIAG(x) 
#endif

#ifndef NULL
#define	NULL	0L
#endif

#ifndef TRUE
#define	TRUE	1
#define	FALSE	0
#endif

long *cookiejar = NULL;

static long __init_cookie(void)
{
	return (long)(*_p_cookies);
}

short
GetCookie(long cookie, long *value)
{
	long r;

	NDIAG(("GetCookie: tag=%ld(%lx)", cookie, cookie));

	r = Ssystem(-1, 0,NULL);
	
	if (!r)
	{
		long val;
		
		NDIAG((" got Ssystem()"));
		r = Ssystem(S_GETCOOKIE, cookie, (long)&val);
		if (!r)
		{
			DIAG((" - return %ld(%lx)", val, val));
			if (value) *value = val;
			return COOKIE_OK;
		}
		else
		{
			DIAG((" - cookie not found (Ssytem() return %lx)", r));
			if (value) *value = 0L;
			return COOKIE_NOTFOUND;
		}
	}
	else
	{
		long *jar;
		
		NDIAG((" no Ssystem()"));
		
		if(cookiejar == NULL)
			cookiejar = (long *)Supexec(__init_cookie);

		if(cookiejar == NULL)
		{
			DIAG((" - no jar!"));
			return (COOKIE_JARNOTEXIST);
		}
		
		jar = cookiejar;
		do
		{
			if (jar[0] == cookie)
			{
				if (value)
					*value = jar[1];
				DIAG((" - return %ld(%lx)", jar[1], jar[1]));
				return COOKIE_OK;
			}
			else
				jar = &(jar[2]);
		}while(jar[-2]);
		DIAG((" - cookie not found"));
		return COOKIE_NOTFOUND;
	}
}

short
SetCookie(long cookie,long value)
{
	long r;

	NDIAG(("SetCookie: tag=%ld(%lx)", cookie, cookie));
	
	r = Ssystem(-1, 0, NULL);
	if (!r)
	{
		NDIAG((" got Ssystem()"));
		r = Ssystem(S_SETCOOKIE, cookie, value);
		if (!r)
		{
			DIAG((" - OK"));
			return COOKIE_OK;
		}
		else
		{
			DIAG((" cookie not found (Ssystem() returns %lx)", r));
			return COOKIE_NOTFOUND;
		}
	}
	else
	{
		long *jar;
		short count = 0;

		if(cookiejar == NULL)
			cookiejar = (long *)Supexec(__init_cookie);

		if (cookiejar == NULL)
			return COOKIE_JARNOTEXIST;
	
		jar = cookiejar;

		do
		{
			count++;									/*	Cookies mitzhlen		*/
			if(jar[0] == cookie)					/*	eigener Cookie schon vorhanden	*/
				return COOKIE_EXISTS;
			else
			{
				jar = &(jar[2]);						/*	weitersuchen	*/
			}
		}while(jar[-2]);							/*	bis zum NULL-Cookie	*/

		if(jar[-1]>count)							/*	Noch gengend Platz fr den eigenen Cookie ?	*/
		{
			jar[0] = 0;								/*	NULL-Cookie neu setzen	*/
			jar[1] = jar[-1];						/*	Anzahl natrlich auch kopieren	*/
			jar[-2] = cookie;						/*	unseren Cookie hineinkopieren	*/
			jar[-1] = value;							/*	unseren Wert auch einfgen	*/
			return COOKIE_OK;					/*	OK zurck melden	*/
		}
		else
			return COOKIE_JARFULL;				/*	Leider kein Platz mehr!	*/
	}
}

short
RemoveCookie(long cookie)
{
	long r;

	NDIAG(("RemoveCookie: tag=%ld(%lx)", cookie, cookie));
	
	r = Ssystem(-1, 0, NULL);
	
	if (!r)
	{
		NDIAG((" got Ssystem()"));

		r = Ssystem(S_DELCOOKIE, cookie, NULL);
		if (!r)
		{
			DIAG((" - OK"));
			return COOKIE_OK;
		}
		else
		{
			DIAG((" - cookie not found (Ssystem() returns %lx)", r));
			return COOKIE_NOTFOUND;
		}
	}
	else
	{
		long *jar;

		NDIAG((" no Ssystem()"));

		if(cookiejar == NULL)
			cookiejar = (long *)Supexec(__init_cookie);

		if(cookiejar == NULL)
		{
			DIAG((" - no jar!"));
			return COOKIE_JARNOTEXIST;
		}
	
		jar = cookiejar;

		do
		{
			if (jar[0] == cookie)					/*	unseren Cookie gefunden	*/
			{
				do
				{
					jar[0] = jar[2];					/*	Cookie kopieren	*/
					jar[1] = jar[3];					/*	Value kopieren	*/
					jar = &(jar[2]);					/*	nchster Cookie	*/
				}while(jar[-2]);					/*	kein NULL-Cookie?	*/
				DIAG((" - OK"));
				return COOKIE_OK;				/*	OK zurck melden	*/
			}
			else
				jar = &(jar[2]);						/*	weitersuchen	*/
		}while(jar[-2]);							/*	bis zum NULL-Cookie	*/
		DIAG((" cookie not found"));
		return COOKIE_NOTFOUND;				/*	Leider Cookie nicht gefunden !	*/
	}
}
