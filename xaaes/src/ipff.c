/*
 * IPFF (c) 1988-2003 by H. Robbers @ Amsterdam
 * Input Parameters in Free Format
 * Unnecessitates the use of stream IO for all these kinds
 * of small parameter files.
 */

/*
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
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

#include <mint/osbind.h>
#include <fcntl.h>

#include "ipff.h"
#include "display.h"
#include "xalloc.h"


/* SYMBOL *(*outer_sym)(char *) = NULL; */
GetSym *outer_sym = NULL;

/* translation and stop table for identifiers  */
static char stpa[] =
{
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	'0','1','2','3','4','5','6','7','8','9',0,0,0,0,0,0,
	0,'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O',
	'P','Q','R','S','T','U','V','W','X','Y','Z',0,0,0,0,'_',
	0,'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o',
	'p','q','r','s','t','u','v','w','x','y','z',0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

/* The above table can be replaced in case of non UK;
 * see ipff_init.
 */

/* translation and stop table for fstr */
static uchar fstpa[256];


static uchar *ln = "";		/* current line */
static uchar *xln = "";		/* next line */
static uchar *stp = stpa;	/* table */
#define max 15			/* default string max */
#define pax 128			/* default path max */
static int mc; 			/* current string max */
static int mp;			/* current path max  */
static bool vul = false;	/* ide aanvullen met ' ' */

char *
ipff_line(long *l)
{
	uchar *t, *s;

	t = s = xln;

	if (*s == 0)
		/* eof */
		return 0;

	while (*s)
	{
		t = s;
		if (*s == '\r' && *(s+1) == '\n')
		{
			*s++ = 0;
			*s++ = 0;
			break;			
		}
		else if (*s == '\n')
		{
			*s++ = 0;
			break;
		}
		s++;
	}

	ln = xln;
	xln = s;
	if (l) *l = t - ln;

	return ln;
}

/* remove trailing spaces */
void
ipff_trail(char *s)
{
	char *t = s;

	while (*t != 0)
		t++;

	while (--t >= s && (*t == ' ' || *t == '\t'))
		*t = 0;
}

char *
ipff_init(int m, int p, int f, char *t, char *l, char **tt)
{
	int i;

	for (i = 0; i < 256;i++) /* every character is allowed */
		fstpa[i] = i;

	fstpa[' '] = 0; /* except these real hard separators */
	fstpa['\t'] = 0;
	fstpa['\r'] = 0;
	fstpa['\n'] = 0;
	fstpa[','] = 0;
	fstpa['|'] = 0;

	if (t >= 0)			/* -1 dont change */
	{				/*  0 use default */
		if (t != 0) stp = t;	/*  value, use that */
		else        stp=stpa;
	}

	if (l > 0)
		ln=l;

	if (p >= 0)
	{
		if (p != 0) mp=p;
		else        mp=pax;
	}
	if (m >= 0)
	{
		if (m != 0) mc=m;
		else        mc=max;
	}

	vul = f;

	if (tt)
		*tt=stp;	/* give identifier translation table (for instance default) */
	xln = ln;
	return ln;		/* give line */
}

void
ipff_in(char *p)
{
	ipff_init(0, 0, 0, NULL, p, NULL);
}

char *
ipff_getp(void)
{
	return ln;
}

void
ipff_putp(char *s)
{
	ln = s;
}

/* skip white space */
int
sk(void)
{
	int c;

	while ((c = *ln) == ' ' 
		|| c == '\t' 
		|| c == '\r' 
		|| c == '\n'
             ) ++ln;

	/* can be zero: end input */
	return c;
}

/* skip analysed character and following white space */
int
skc(void)
{
	if (*ln) ++ln;
	return sk();
}

/* skip only analysed character */
int
sk1(void)
{
	if (*ln)
		return *(++ln);
	return 0;
}

/* give delimited string */
int
lstr(char *w, unsigned long lim)
{
	int i = 0, c = *ln;
	char *s = (char *)&lim;

	while  (i < mc)
	{
		c = *ln;

		if (c == 0     ) break;	/* einde input */
		if (c == *(s+3)) break;
		if (c == *(s+2)) break;
		if (c == *(s+1)) break;
		if (c == * s   ) break;

		*w++ = c;
		ln++;
		i++;
	}

	if (vul)
	{
		while (i++ < mc)
			*w++ = ' ';
	}

	*w = 0;

	/* return stop ch for analysis */
	return c;
}

/* give string */
int
ide(char *w)
{
	int i, c = *ln, vc;

	i = 0;
	while  (i < mc)
	{
		c = *ln;
		if (c == 0) break;	/* einde input */

		vc = stp[(uchar)c];
		if (vc == 0) break;	/* stop ch     */

		*w++ = vc;
		ln++;
		i++;
	}

	if (vul)
	{
		while (i++ < mc)
			*w++ = ' ';
	}

	*w = 0;

	/* return stop ch for analysis */
	return c;
}

static int
brace(void)
{
	switch (*ln)
	{
		case '\'':
		case '"':
			return *ln++;
	}

	return 0;
}

bool
infix(void)
{
	int c = sk();
	return (c == ',' || c == '|');
}

/* only there to avoid long mul */
int
idec(void)
{
	int n = 0; bool t = false;

	if (*ln == '-')
	{
		ln++;
		t = true;
	}

	while (*ln >= '0' && *ln <= '9')
		n = 10 * n + *ln++ - '0';

	if (t) return -n;
	else   return  n;
}

long
dec(void)
{
	long n = 0; bool t = false;

	if (*ln == '-')
	{
		ln++;
		t = true;
	}

	while (*ln >= '0' && *ln <= '9')
		n = 10 * n + *ln++ - '0';

	if (t) return -n;
	else   return  n;
}

long
hex(void)
{
	long n = 0; int c; bool t = false;

	if (*ln == '-')
	{
		ln++;
		t = true;
	}

	while ((c = *ln) != 0)
	{
		if (c >= '0' && c <= '9')
			n = 16 * n + (c - '0');
		else if ( c >= 'A' && c <= 'F')
			n=16 * n + (c - 'A' + 10);
		else if ( c >= 'a' && c <= 'f')
			n=16 * n + (c - 'a' + 10);
		else
			break;
		ln++;
	}

	if (t) return -n;
	else   return  n;
}

long
oct(void)
{
	long n = 0; bool t = false;

	if (*ln == '-')
	{
		ln++;
		t = true;
	}

	while (*ln >= '0' && *ln <= '7')
		n = 8 * n + *ln++ - '0';

	if (t) return -n;
	else   return  n;
}

long
bin(void)
{
	long n = 0; bool t = false;

	if (*ln == '-')
	{
		ln++;
		t = true;
	}

	while (*ln >= '0' && *ln <= '1')
		n = 2 * n + *ln++ - '0';

	if (t) return -n;
	else   return  n;
}

/* 0 = true, anything else is false */
bool
truth(void)
{
	bool rev = 0, t = 1;
	SYMBOL *s;
	char v[128];

	sk();

	if (*ln == '!')
	{
		rev = 1;
		skc();
	}

	if (*ln >= '0' && *ln <= '9')
		t = (idec() ? 0 : 1);
	else
	{
		ide(v);
		if (*v)
		{
			if (strcmp(v, "not") == 0)
				return truth() == 0 ? 1 : 0;
			if (strcmp(v, "true") == 0)
				t = 0;
			else if (strcmp(v, "false") == 0)
				t = 1;
			else if (strcmp(v, "yes") == 0)
				t = 0;
			else if (strcmp(v, "no") == 0)
				t = 1;
			else
			{
				s = find_sym(v, outer_sym);
				if (s)
				{
					if (s->mode == Bit)
						t = s->val;
					else if (s->mode == String)
						t = s->s ? 0 : 1;
					else
						t = s->val ? 0 : 1;
				}
			}
		}
	}
	return rev == 0 ? t : (t == 0 ? 1 : 0);
}

/* Check if the string conforms to the
 * character set for identifiers.
 */
bool
is_ide(char *s)
{
	while (*s)
	{
		if (stpa[(uchar)*s] == 0) /* same table as for ide() */
			return false;
		else
			s++;
	}
	return true;
}

/* give a path regarding delimiters
 * p must be large enough (see ipff_init)
 */
SYMBOL *
fstr(char *p, int cas, int lval)
{
	char *start = p;
	SYMBOL *s;

	while (*ln && p-start < mp-1)
	{
		int c = sk();
		bool tostr = c == '#';

		if (tostr)
			c = skc();

		if (c >= '0' && c <= '9')
		{
			if (tostr)
				*p++ = dec();
			else
				p += sdisplay(p, "%ld", dec());
		}
		else
		{
			char *first = p;
			int apo = brace();
			while (*ln && *ln != apo && p-start < mp-1)
			{
				c = *ln;
				if (!apo)
					if ((c = fstpa[c & 0xff]) == 0)
						break;
				*p++ = cas ? cas > 0 ? toupper(c) : tolower(c) : c;
				ln++;
			}

			if (apo && *ln == apo)
			{
				ln++;
			}
			else
			{
				*p = 0;
				if (   !lval
				    && is_ide(first)
				    && (s = find_sym(first, outer_sym)) != NULL)
				{
					if (s->mode == String)
					{
						strcpy(first, s->s);
						p = first + strlen(first);
					}
					else if (s->mode == Bit)
					{
						first += sdisplay(first, "%s", s->val ? "false" : "true");
						p = first;
					}
					else if (s->mode == Number)
					{
						if (tostr)
						{
							*first = s->val;
							p = first + 1;
						}
						else
						{		
							first += sdisplay(first, "%ld", s->val);
							p = first;
						}
					}
				}
			}
		}

		if (sk() != '+')
			break;
		skc();
	}

	*p = 0;

	/* display("fstr: '%s'\n", start); */

	if (lval && is_ide(start))
		return find_sym(start, outer_sym);

	return NULL;
}

void
nstr(char *w)	/* give rest of line */
{
	int i,c;

	i = 0;
	while  (i < mc)
	{
		c = *ln;
		if (   c == 0 
		    || c == '\r'
		    || c == '\n') break;	/* einde input */

		*w++ = c;
		ln++;
		i++;
	}

	*w = 0;
}

char *
Fload(const char *name, int *fh, long *l)
{
	long pl,fl;
	char *bitmap=0L;

	*l =0;
	fl = *fh; /* if already opened, *fh is handle */

	if (name)
		/* if not already or want to open */
		fl = Fopen(name,O_RDONLY);

	if (fl > 0)
	{
		fl &= 0xffff;
		pl = Fseek(0L, fl, 2); /* seek end */
		Fseek(0L, fl, 0); /* seek start */

		bitmap = xmalloc(pl+1, 200);
		if (bitmap)
		{
			Fread(fl, pl, bitmap);
			Fclose(fl);
			*l = pl;
			*(bitmap + pl) = 0;	/* make it a C string */
		}
	}

	*fh = fl;
	return bitmap;
}

static SYMBOL *symbol_table = NULL;

SYMBOL *
new_sym(char *name, int atype, MODE mode, char *s, long v)
{
	SYMBOL *new = NULL;
	char *vs, *n = xmalloc(strlen(name) + 1, 201);

	if (n)
	{
		strcpy(n, name);
		new = xcalloc(1, sizeof(*new), 202);
		if (new)
		{
			new->assigntype = atype;
			new->name = n;
			if (mode != String)
			{
				new->val = v;
				new->mode = mode;
			}
			else
			{
				vs = xmalloc(strlen(s) + 1, 203);
				if (vs)
				{
					new->s = vs;
					new->mode = mode;
					strcpy(vs, s);
				}
				else
				{
					free(n);
					return NULL;
				}
			}
			new->next = symbol_table;
			symbol_table = new;
		}
	}
	return new;
}

void
free_sym(void)
{
	SYMBOL *t;

	t = symbol_table;
	while(t)
	{
		SYMBOL *next = t->next;

		if (t->name)
			free(t->name);

		if (t->s)
			free(t->s);

		free(t);
		t = next;
	}
}

void
list_sym(void)
{
	SYMBOL *t;

	t = symbol_table;
	while (t)
	{
		display("ty=%d, ide='%s' s='%s', %ld\n",
			t->mode, t->name, t->s ? t->s : "", t->val);
		t = t->next;
	}
}

SYMBOL *
find_sym(char *name, GetSym *outer_sym_getter)
{
	SYMBOL *t;

	t = symbol_table;
	while (t)
	{
		/* display("'%s' :: '%s'\n", t->name, name); */
		if (strcmp(t->name, name) == 0)
			break;
		t = t->next;
	}

	if (outer_sym_getter && t == NULL)
		t = outer_sym_getter(name);

	return t;
}

int
assign(void)
{
	char *p = ipff_getp();
	int c = sk();
	if (c == 'i')
	{
		if (sk1() == 's' && sk1() == ' ')
			return 0;		/* constant */
	}
	else if (c == ':')
	{
		if (sk1() == '=')
			return 1;
	}
	ipff_putp(p);
	return -1;
}
