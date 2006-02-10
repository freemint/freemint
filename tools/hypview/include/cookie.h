/*
	Cookie-Routinen
	(c) 25.05.1999 Philipp Donz
*/
short GetCookie(long cookie,long *value);
short SetCookie(long cookie,long value);
short RemoveCookie(long cookie);

/*
 *		Error-Codes:
 */
#define	COOKIE_OK		0
#define	COOKIE_JARNOTEXIST	1
#define	COOKIE_JARFULL		2
#define	COOKIE_NOTFOUND		3
#define	COOKIE_EXISTS		4

