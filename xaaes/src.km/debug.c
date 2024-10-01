/*
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

#define PROFILING	0
#include "debug.h"
#include "xa_types.h"
#include "xa_global.h"

#include "k_mouse.h"

#ifdef BOOTLOG

static inline void
write_bootlog(char *t, short l)
{
	struct file *fp;

	if (C.bootlog_path[0])
	{
		fp = kernel_open(C.bootlog_path, O_RDWR, NULL,NULL);

		if (!fp)
			fp = kernel_open(C.bootlog_path, O_RDWR|O_CREAT|O_TRUNC, NULL,NULL);
		else
			kernel_lseek(fp, 0, SEEK_END);
		if (fp) {
			kernel_write(fp, t, l);
			kernel_close(fp);
		}
	}
}
#endif

void _cdecl
bootlog(short disp, const char *fmt, ...)
{
#ifdef BOOTLOG
	char buf[512];
	va_list args;
	long l;
	int lvl;
	short dlvl = disp >> 1, llvl = C.loglvl;
	disp = (disp & 1) | (C.loglvl & 2);

	if( !disp && llvl <= dlvl )
		return;

	lvl = DEBUG_LEVEL;
	DEBUG_LEVEL = 0;

	va_start(args, fmt);
	l = vsprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	if( llvl > dlvl )
	{
		buf[l] = '\n';
#if GENERATE_DIAGS
		if (D.debug_file)
		{
			kernel_write(D.debug_file, buf, l+1);
		}
#endif
		write_bootlog(buf, l+1);
	}
	if (disp)
	{
		buf[l] = '\r';
		buf[l+1] = '\n';
		buf[l+2] = '\0';
		c_conws(buf);
	}
	DEBUG_LEVEL = lvl;
#else
	UNUSED(disp);
	UNUSED(fmt);
#endif
}


void _cdecl
display(const char *fmt, ...)
{
	char buf[512];
	va_list args;
	long l;

	va_start(args, fmt);
	l = vsprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	DEBUG((buf));

	buf[l] = '\n';
#if GENERATE_DIAGS
	if (D.debug_file)
	{
		buf[l] = '\n';
		kernel_write(D.debug_file, buf, l+1);
	}
#endif
#ifdef BOOTLOG
	if( C.loglvl & 1 )
		write_bootlog(buf, l+1);
#endif
#if 1
	buf[l] = '\r';
	buf[l+1] = '\n';
	buf[l+2] = '\0';
	c_conws(buf);
#endif
}

void _cdecl
ndisplay(const char *fmt, ...)
{
	char buf[512];
	va_list args;
	long l;

	va_start(args, fmt);
	l = vsprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	DEBUG((buf));

#if GENERATE_DIAGS
	if (D.debug_file)
	{
		kernel_write(D.debug_file, buf, l);
	}
#endif
#ifdef BOOTLOG
	if( C.loglvl & 1 )
		write_bootlog(buf, l);
#endif
#if 1
	buf[l] = '\0';
	c_conws(buf);
#endif
}

#if PROFILING

#define PRFUSETRAP	1
#if PRFUSETRAP
#ifdef trap_14_w
#undef trap_14_w	/* "redefined" warning */
#endif

#include <mintbind.h>	/* Tgettimeofday */
#else
#define Tgettimeofday	_t_gettimeofday
#endif

#include "debug.h"

struct pr_info{
	long ms, sms, calls;
	char *name;
	int len;
};


/*
 * accumulate usages for names
 *
 * return rv
 *
 */
#define N_PRINF	32
int prof_acc( const char *name, enum prof_cmd cmd, int rv )
{
	static struct pr_info PrInfo[N_PRINF];
	static struct pr_info MePrInfo;
	static int longest_name = 0;
	int i, f = -1, l;
  struct timeval tv;
  long ms, sms;

	Tgettimeofday( &tv, 0 );

	if( name != NULL ){
		for( i = 0; i < N_PRINF; i++ )
			if( PrInfo[i].name == name )
				break;

		if( i < N_PRINF && PrInfo[i].name != 0 )
			 f = i;
	}

	switch( cmd ){
	case 0:	/*init all*/
		for( i = 0; i < N_PRINF; i++ ){
			PrInfo[i].name = 0;
			PrInfo[i].ms = 0;
			PrInfo[i].calls = 0;
			longest_name = 0;
			MePrInfo.ms = MePrInfo.calls = 0;
		}
	break;
	case 1:	/*init name*/
		if( f >= 0 ){
			PrInfo[f].ms = 0;
			PrInfo[f].name = name;
			if( (l=strlen( name )) > longest_name )
				longest_name = l;

			PrInfo[f].len = l;
			PrInfo[f].calls = 0;
		}

	break;
	case 2:	/* start name, init if new */
		if( f == -1 ){

			for( i = 0; i < N_PRINF && PrInfo[i].name; i++ );

			if( i == N_PRINF ){
				profile( "%s: list full.", name );
				break;
			}
			PrInfo[i].name = name;
			if( (l=strlen( name )) > longest_name )
				longest_name = l;
			PrInfo[i].len = l;
			PrInfo[i].ms = 0;
			PrInfo[i].calls = 0;
			f = i;
		}
		if( f >= 0 ){
			PrInfo[f].sms = tv.tv_sec * 1000L + tv.tv_usec / 1000L;
		}
	break;
	case 3:	/* stop name */
		if( f >= 0 ){
			ms = tv.tv_sec * 1000L + tv.tv_usec / 1000L;
			PrInfo[f].ms += ms - PrInfo[f].sms;
			PrInfo[f].calls++;
		}
	break;
	case 4:	/* print all */
	{
		char nbuf[128];
		profile( "--usage-summary---" );
		l = longest_name + 3;
		if( l >= sizeof(nbuf) -1)
			l = sizeof(nbuf) - 2;
		for( f = 4, i = 0; f < l; f++)
			nbuf[i++] = ' ';
		nbuf[i++] = ':';
		nbuf[i] = 0;
		profile( "name%susage  calls", nbuf );	/* header */
		for( i = 0; i < N_PRINF && PrInfo[i].name; i++ ){
			strncpy( nbuf, PrInfo[i].name, sizeof(nbuf)-1 );
			for( f = PrInfo[i].len; f < l; )
				nbuf[f++] = ' ';
			nbuf[f++] = ':';
			nbuf[f] = '\0';
			profile( "%s%4ld%6ld", nbuf, PrInfo[i].ms, PrInfo[i].calls );
		}
		profile( "--\n     --profiling needed %4ldms\t%4ld calls--", MePrInfo.ms, MePrInfo.calls );
		profile( "--end of summary--" );
	}
	break;
	case 5:	/* drop name */
		if( f >= 0 ){
			PrInfo[f].name = 0;
		}
		/* update longest_name */
	break;
	}
	MePrInfo.calls++;
	sms = tv.tv_sec * 1000L + tv.tv_usec / 1000L;
	Tgettimeofday( &tv, 0 );
	ms = tv.tv_sec * 1000L + tv.tv_usec / 1000L;
	MePrInfo.ms += ms - sms;
	return rv;
}

/*
 * simple profiling
 * output a message along with elapsed ms since last call to "xaaesprf.log"
 *
 * t = 0 closes the file
 */
void
profile( const char *t, ...)
{
  static struct timeval tv1 = {0,0};
  struct timeval tv2;
  static struct file *fp = 0;
  static char *Name = "xaaesprf.log";
  char ebuf[256];
  int l;
  long usec, sec;
	va_list argpoint;

	Tgettimeofday( &tv2, 0 );

	if( t == 0 ){
		if( fp ){
			char *msg = "--profile closed--\n";
			kernel_write( fp, "--profile started--\n", strlen(msg) );
			kernel_close(fp);
			fp = 0;
		}
		return;
	}

  if( Name == 0 )
  	return;	/* could not open */

	if( !fp ){
		char *msg = "--profile started--\n";

		sprintf( ebuf, sizeof(ebuf), "%s%s", C.start_path, Name );
		fp = kernel_open( ebuf, O_RDWR, NULL,NULL);
		if( !fp )
			fp = kernel_open( ebuf, O_RDWR|O_CREAT|O_TRUNC, NULL,NULL);
		else
			kernel_lseek(fp, 0, SEEK_END);
		if( !fp ){
			BLOG((0, "profile: cannot open %s", ebuf ));
			Name = 0;
			return;
		}
		kernel_write( fp, msg, strlen(msg) );
	}
	if( tv1.tv_sec > 0 ){

	  usec = (tv2.tv_usec - tv1.tv_usec) / 1000L;
	  sec = tv2.tv_sec - tv1.tv_sec;
	  if( usec < 0 ){
	  	sec--;
	  	usec += 1000L;
	  }

	  l = sprintf( ebuf, sizeof(ebuf), "%4ld:", sec * 1000 + usec );
	}
	else
		l = 0;

	va_start(argpoint, t);
	l += vsprintf( ebuf+l, sizeof(ebuf) - l, t, argpoint );
	va_end(argpoint);

	ebuf[l++] = '\n';
	ebuf[l] = 0;

	kernel_write( fp, ebuf, l );

	tv1 = tv2;

}

#endif	/* PROFILING*/

#if GENERATE_DIAGS

/* debugging catagories & data */
struct debugger D;

const char *D_su = "    Sema %c up\n";
const char *D_sd = "    Sema %c down: %ld\n";
const char *D_sr = "  ::  %ld\n";
const char *D_cl = "CONTROL: %d::%d, %d::%d, %d::%d\n";
const char *D_fl = "%s,%d: ";
const char *D_flu = "%ld,%d - %s,%d: ";

void
show_bits(unsigned short data, const char *prf, const char *const t[], char *x)
{
	int i = 0;
	*x = '\0';
	if (data)
	{
		strcpy(x, prf);
		while (data)
		{
			if (data & 1)
			{
				strcat(x, t[i]);
				strcat(x, "|");
			}
			data >>= 1;
			i++;
		}
		i = strlen(x) - 1;
		if (x[i] == '|')
			x[i] = ',';
	}
}

const char *
w_owner(struct xa_window *w)
{
	if (w)
	{
		if (w->owner)
			return c_owner(w->owner);
		return "->NULL";
	}
	return "NULL";
}

const char *
c_owner(struct xa_client *c)
{
	static char buf[512];

	if (c)
	{
		sprintf(buf, sizeof(buf), "'%s'", c->name);
		return buf;
	}
	return "NULL";
}

const char *
t_owner(XA_TREE *t)
{
	if (t)
	{
		if (t->owner)
			return c_owner(t->owner);
		return "->NULL";
	}
	return "NULL";
}

void
diags(const char *fmt, ...)
{
#if GENERATE_DIAGS
	char buf[512];
	va_list args;
	long l;

	va_start(args, fmt);
	l = vsprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	DEBUG((buf));

	if (D.debug_file)
	{
		buf[l++] = '\n';
		kernel_write(D.debug_file, buf, l);
	}
#endif
}

void
diaga(const char *fmt, ...)
{
	char buf[512];
	va_list args;
	long l;

	va_start(args, fmt);
	l = vsprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	DEBUG((buf));

#if GENERATE_DIAGS
	if (D.debug_file)
	{
		buf[l++] = '\n';
		kernel_write(D.debug_file, buf, l);
	}
#endif
}

void
diag(enum debug_item item, struct xa_client *client, const char *t, ...)
{
	enum debug_item *point = client ? client->options.point : D.point;
	short b, x, y;

	if (D.debug_level == 4
	    || (    (D.debug_level >= 2 && point[item])
	        && !(D.debug_level == 3 && client == NULL)))
	{
		char line[512];
		long l = 0;
		va_list argpoint;

		va_start(argpoint, t);

		if (D.point[D_v])
		{
			check_mouse(client, &b, &x, &y);

			l += sprintf(line+l, sizeof(line)-l, "[B%d]", b);

#if 0
			if (mu_button.got || mu_button.have)
				l += sprintf(line+l, sizeof(line)-l, "[%c%c]",
					     mu_button.have ? 'H' : ' ',
					     mu_button.got ? 'G' : ' ');
#endif
		}

		{
			struct proc *update_lock = update_locked();
			struct proc *mouse_lock = mouse_locked();

			l += sprintf(line+l, sizeof(line)-l, "(Pid %ld)", p_getpid());

			if (mouse_lock)
				l += sprintf(line+l, sizeof(line)-l, "[M/%d]",
						mouse_lock->pid);

			if (update_lock)
				l += sprintf(line+l, sizeof(line)-l, "[U%d]",
						update_lock->pid);
		}

		if (client == NULL)
			l += sprintf(line+l, sizeof(line)-l, " ~ ");

		l += vsprintf(line+l, sizeof(line)-l, t, argpoint);

		va_end(argpoint);

		DEBUG((line));

		if (D.debug_file)
		{
			line[l++] = '\n';
			kernel_write(D.debug_file, line, l);
		}
	}
}

#endif /* GENERATE_DIAGS */
