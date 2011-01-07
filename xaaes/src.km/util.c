/*
 * $Id$
 *
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann & O.Skancke
 *
 * A multitasking AES replacement for FreeMiNT
 *
 * This file is part of XaAES.
 *
 * XaAES is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * XaAES is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "util.h"
#include "debug.h"

int
get_drv(const char *p)
{
	if (*(p + 1) == ':')
	{
		int c = *p;
		if (c >= 'a' && c <= 'z')
			return c - 'a';
		if (c >= 'A' && c <= 'Z')
			return c - 'A';
		if (c >= '0' && c <= '9')
			return (c - '0') + ('z' - ('a' - 1));
	}
	return -1;
}

void
fix_path(char *path)
{
	char c;

	if (!path)
		return;

	while ((c = *path)) {
		if (c == '\\')
			*path = '/';
		path++;
	}
}

void
strip_fname(const char *path, char *newpath, char *fname)
{
	const char *s, *d = NULL;
	char c;

	s = path;
	c = *s;

	while (c) {
		if (c == '/' || c == '\\') {
			d = s + 1;
// 			if (c == '\\')
// 				*s = '/';
		}
		c = *++s;
	}
	if (d && d != s) {
		int slen = d - path;

		if (fname)
			strcpy(fname, d);
		if (newpath) {
			if (newpath != path)
				strncpy(newpath, path, slen);
			newpath[slen] = '\0';
		}
	}
}

int
drive_and_path(char *fname, char *path, char *name, bool n, bool set)
{
	int t; char *tn = fname;
	int drive = get_drv(fname);

	if (drive >= 0)
		tn += 2;

	strcpy(path, tn);

	if (n)
	{
		/* Seperate path & name */
		t = strlen(path)-1;
		while (t >= 0 && path[t] != '\\')
			t--;

		if (path[t] == '\\')
		{
			path[t] = '\0';
			if (name)
				strcpy(name, path + t + 1);
		}
		else
			if (name)
				strcpy(name, path);
	}

	if (set)
	{
		if (drive >= 0)
			d_setdrv(drive);

		d_setpath(path);
	}

	return drive;
}

void
set_drive_and_path(char *fname)
{
	char buf[256];
	int t; char *tn;
	int drive;

	strcpy(buf, fname);
	tn = buf;
	drive = get_drv(tn);

	if (drive >= 0)
		tn += 2;

	/* Seperate path & name */
	t = strlen(tn) - 1;
	while (t >= 0 && tn[t] != '\\')
		t--;

	if (tn[t] == '\\')
	{
		tn[t] = '\0';
	}
	if (!*tn)
	{
		tn[0] = '\\';
		tn[1] = '\0';
	}
	DIAGS(("set_drive_and_path %d '%s'", drive, tn));
	if (drive >= 0)
		d_setdrv(drive);

	d_setpath(tn);
}

void
get_drive_and_path(char *path, short plen)
{
	int drv;

	drv = d_getdrv();
	path[0] = (char)drv + 'a';
	path[1] = ':';
	d_getpath(path + 2, 0);
	drv = strlen(path);
	if (path[drv - 1] != '\\' && path[drv - 1] != '/')
	{
		path[drv++] = '\\';
		path[drv] = '\0';
	}
}

void
strnupr(char *s, int n)
{
	int f;

	for (f = 0; f < n; f++)
		s[f] = toupper(s[f]);
}


struct xa_file{
	struct file *k_fp;
	char buf[1024];
	char *p, *p1;
};

XA_FILE *xa_fopen( char *fn, int rwmd )
{
	XA_FILE *ret;
	long err;
	struct file *fp = kernel_open( fn, rwmd, &err, NULL );
	if( !fp )
	{
		BLOG((0,"xa_fopen: cannot open %s", fn));
		return 0;
	}
	ret = kmalloc( sizeof( XA_FILE ) );
	if( !ret )
	{
		BLOG((0,"xa_fopen: malloc for %s", fn));
		return 0;
	}
	ret->k_fp = fp;
	ret->p1 = ret->p = 0;//ret->buf;

	return ret;
}

void xa_fclose( XA_FILE *fp )
{
	if( !fp )
		return;
	kernel_close( fp->k_fp );
	kfree( fp );
}

/*
 * simple version!
 */
int xa_writeline( char *buf, long l, XA_FILE *fp )
{
	if( l <= 0 || buf == 0 || (kernel_write( fp->k_fp, buf, l ) <= 0) )
		return 0;
	return kernel_write( fp->k_fp, "\n", 1 );
}

char *xa_readline( char *buf, long size, XA_FILE *fp )
{
	long err = 1;
	int cr = 0, offs;
	char *ret = 0;


	if( fp->k_fp )
	{

		if( !fp->p )
		{
			err = kernel_read( fp->k_fp, fp->buf, sizeof(fp->buf)-1 );

			if( err <= 0 )
			{
				return 0;
			}

			fp->buf[err] = 0;

			fp->p1 = fp->p = fp->buf;
		}
		else fp->p++;

		for( ;; fp->p++ )
		{
			if( !*fp->p )
			{
				offs = fp->p - fp->p1;
				if( offs > 0 )
				{
					memcpy( fp->buf, fp->p1, offs );
				}
				fp->p1 = fp->buf;

				fp->p = fp->buf + offs;

				err = kernel_read( fp->k_fp, fp->p, sizeof(fp->buf) - ( fp->p - fp->buf ) - 1 );
				if( err <= 0 )
					break;
				fp->p[err] = 0;
			}
			if( *fp->p == '\n' || (cr=(*fp->p == '\r')) )
				break;
		}

		*fp->p = 0;

		if( size > 0 && buf )
			strncpy( buf, fp->p1, size );

		ret = fp->p1;	/* !! */


		if( cr )
		{
			cr = 0;
			if( !*(fp->p+1) )
			{
				kernel_read( fp->k_fp, fp->p, 1 );	// skip the \n
				*fp->p = 0;
				err = 0;
			}
			else{
				fp->p++;
			}
		}

		if( err )
			fp->p1 = fp->p+1;

		if( err <= 0 )
			fp->p = 0;
		return ret;

	}

	return 0;
}


#if 0
bool
xa_invalid(int which, int pid, void *addr, long size, bool allowNULL)
{
	if (allowNULL && addr == NULL)
		return false;

	{
		long rep;

		rep = Pvalidate(pid, addr, size, NULL);
		DIAGS(("[%d]xa_invalid::%ld for %d: %ld(%lx) + %ld(%lx) --> %ld(%lx)",
			which,
			rep,
			pid,
			addr,addr,
			size,size,
			(long)addr+size,
			(long)addr+size ));

		return rep != 0;
	}
}
#endif
