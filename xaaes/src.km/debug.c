/*
 * $Id$
 *
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann
 *
 * A multitasking AES replacement for MiNT
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

#include "xa_types.h"
#include "xa_global.h"

#include "k_mouse.h"


void
fdisplay(struct file *log, const char *fmt, ...)
{
	char buf[512];
	va_list args;
	long l;

	va_start(args, fmt);
	l = vsprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	DEBUG((buf));

	if (log)
	{
		buf[l++] = '\n';
		kernel_write(log, buf, l);
	}
}

void
display(const char *fmt, ...)
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
show_bits(unsigned short data, char *prf, char *t[], char *x)
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

char *
w_owner(struct xa_window *w)
{
	static char buf[128];

	if (w)
		if (w->owner)
			sprintf(buf, sizeof(buf), "'%s'", w->owner->name);
		else
			sprintf(buf, sizeof(buf), "->NULL");
	else
		sprintf(buf, sizeof(buf), "NULL");

	return buf;
}

char *
c_owner(struct xa_client *c)
{
	static char buf[512];

	if (c)
		sprintf(buf, sizeof(buf), "'%s'", c->name);
	else
		sprintf(buf, sizeof(buf), "NULL");

	return buf;
}

char *
t_owner(XA_TREE *t)
{
	static char buf[512];

	if (t)
		if (t->owner)
			sprintf(buf, sizeof(buf), "'%s'", t->owner->name);
		else
			sprintf(buf, sizeof(buf), "->NULL");
	else
		sprintf(buf, sizeof(buf), "NULL");

	return buf;
}

#if 0
/* HR: once used to debug a window_list corruption.
       also a example of how to use this trace facility. */
long
xa_trace(char *t)
{
	long l = 0;
	return l;
}
#endif

void
DeBug(enum debug_item item, struct xa_client *client, char *t, ...)
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

#if 0
		/* HR: xa_trace can by anything, anywhere.
		 *	Making debug a function, made a very primitive variable tracer
		 *	possible. Found some very nasty bugs by pinpointing the
		 *	offence between 2 debug call's.
		 */
		l += xa_trace(line);
#endif
		if (D.point[D_v])
		{
			check_mouse(client, &b, &x, &y);

			l += sprintf(line+l, sizeof(line)-l, "[B%d]", b);

			if (mu_button.got || mu_button.have)
				l += sprintf(line+l, sizeof(line)-l, "[%c%c]",
					     mu_button.have ? 'H' : ' ',
					     mu_button.got ? 'G' : ' ');
		}

		{
			struct xa_client *update_lock = update_locked();
			struct xa_client *mouse_lock = mouse_locked();

			if (mouse_lock)
				l += sprintf(line+l, sizeof(line)-l, "[M/%d]",
						mouse_lock->p->pid);

			if (update_lock)
				l += sprintf(line+l, sizeof(line)-l, "[U%d/%d]",
						update_lock->p->pid);
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

void
display_env(char **env, int which)
{
	if (D.debug_level > 2 && D.point[D_shel])
	{
		if (which == 1)
		{
			char *e = *env;
			display("Environment as superstring:\n");
			while (*e)
			{
				display("%lx='%s'\n", e, e);
				e += strlen(e)+1;
			}
		}
		else
		{
			display("Environment as row of pointers:\n");
			while (*env)
			{
				display("%lx='%s'\n", *env, *env);
				env++;
			}
		}
	}
}

#endif /* GENERATE_DIAGS */
