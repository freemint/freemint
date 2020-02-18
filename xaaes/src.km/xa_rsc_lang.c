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
#include "xa_types.h"
#include "xa_aes.h"
#include "xa_global.h"

#include "draw_obj.h"
#include "trnfm.h"
#include "obtree.h"
#include "xa_appl.h"
#include "xa_rsrc.h"
#include "xa_shel.h"
#include "util.h"

#include "mint/fcntl.h"
#include "mint/stat.h"
#include "mint/signal.h"

#include "xa_rsc_lang.h"

#ifndef OF_NOTRANS
# define OF_NOTRANS 0x8000
#endif


#define LF_OFFS 3
#define LF_SEPCH	'_'
#define LF_COMMCH	'#'
#define LF_SEPSTR	"_"

#define RSL_MAX_ERRORS	10				/* maximum number of errors before translator gives up */


static int rsl_errors;					/* maximum alerts for invalid rsl-file */
static int reported_skipped;
static long rsl_lno;

enum
{
	NOT_FOUND = 0,
	OFOUND = -1,
	TFOUND = 1
};


/*
 * md=REPLACE buf is char**
 *            l=-1: don't translate
 */
static short rsc_lang_file_replace(XA_FILE *fp, char **buf, bool *strip)
{
	char lbuf[256];
	char *p = 0;
	char *in = *buf;
	int len = strlen(in);
	int found;
	short blen = 0;
	short clen;
	short cmplen;

	if (!len)
		return 0;
	/* ignore trailing blanks for comparison */
	for (clen = len - 1; clen && in[clen] == ' '; clen--)
		;
	clen++;

	for (found = NOT_FOUND, lbuf[0] = 0; found == NOT_FOUND && lbuf[0] != -1;)
	{
		for (lbuf[0] = 0; lbuf[0] != LF_SEPCH;)
		{
			if (!xa_readline(lbuf, sizeof(lbuf) - 1, fp))
			{
				lbuf[0] = -1;
				break;
			}
			rsl_lno++;
			if (lbuf[0] == LF_COMMCH)
				continue;
			if (lbuf[0] == '\0')
				continue;

			if (lbuf[0] == LF_SEPCH)
				break;
			if (lbuf[1] == '\0' ||
				(lbuf[LF_OFFS - 1] != ':' && lbuf[LF_OFFS - 1] != '+' && lbuf[LF_OFFS - 1] != ';'))
			{
				BLOG((0, "%ld:'%s': language id expected", rsl_lno, lbuf));
				continue;
			}
			cmplen = clen;

			p = lbuf + LF_OFFS;
			blen = strlen(p);

			/* input-length */
			if (lbuf[blen + LF_OFFS - 1] == '@' && lbuf[blen + LF_OFFS - 2] == ' ')
			{
				blen--;
				cmplen = len;
			}
			lbuf[blen + LF_OFFS] = 0;

			if (found == NOT_FOUND)
			{
				if (strncmp(lbuf, "nn", 2) == 0)
				{
					if ((*p == '-' && *in == '-') || strncmp(p, in, cmplen) == 0)
					{
						found = OFOUND;
						if (lbuf[LF_OFFS - 1] == ';')	/* dont translate this */
							break;
						if (lbuf[LF_OFFS - 1] == '+')	/* realloc */
							*strip = false;
					} else if (reported_skipped == 0)
					{
						BLOG((0, "%ld:'%s' skipped (expected:'%s')", rsl_lno, p, in));
						reported_skipped = 1;
					}
				}
			} else if (strncmp(lbuf, cfg.lang, 2) == 0)
			{
				/* copy shorter length */
				if (blen > len && *strip)
					blen = len;
				found = TFOUND;
				break;					/* translation found */
			}
		}
	}

	if (found == TFOUND && p && lbuf[0] != -1 && lbuf[0] != LF_SEPCH)	/* found */
	{
		if (blen > len && *strip == false)
		{
			in = xa_strdup(p); /* FIXME: will be leaked */
			*buf = in;
		} else
		{
			if (blen > 0)
			{
				in[blen] = 0;
				memcpy(in, p, blen);
			} else
			{
				blen = len;
			}
		}
	} else
	{
		blen = clen;
	}

	while (lbuf[0] != LF_SEPCH)
	{
		if (!xa_readline(lbuf, sizeof(lbuf) - 1, fp))
		{
			break;
		}
		rsl_lno++;
	}
	switch (found)
	{
	case TFOUND:
		return blen;
	case OFOUND:
		return -blen;
	default:
		return 0;
	}
}


static short translate_string(struct xa_client *client, XA_FILE * rfp, char **p, bool *strip)
{
	short blen;
	short j;

	UNUSED(client);
	if (!p)
		return 0;

	for (blen = j = 0; j < 2 && !blen; j++)
	{
		if ((blen = rsc_lang_file_replace(rfp, p, strip)))
			break;						/* found */
		if (1 || rsl_errors++ < RSL_MAX_ERRORS)
		{
			BLOG((0, "translate:'%s':rewind", *p));
			xa_rewind(rfp);
			rsl_lno = 1;
			reported_skipped = -1;
		} else
			break;						/* give up */
	}
	if (!blen)
	{
		BLOG((0, "%s:translate: item '%s': not found", client->name, *p));
	}
	return blen;
}


short rsc_trans_rw(struct xa_client *client, XA_FILE *rfp, char **txt, bool *strip)
{
	short blen = 0;
	char buf[256];

	if (!rfp || !txt || !*txt || !**txt)
		return 0;

	if (client->options.rsc_lang == WRITE)
	{
		int len = sprintf(buf, sizeof(buf), "nn:%s", *txt);

		if (*(buf + len - 1) == ' ')
		{
			*(buf + len) = '@';
			len++;
			*(buf + len) = 0;
		}
		blen = len;
		rsc_lang_file_write(rfp, buf, len);
		rsc_lang_file_write(rfp, LF_SEPSTR, 1);
	} else if (client->options.rsc_lang == READ)
	{
		blen = translate_string(client, rfp, txt, strip);
	}

	return blen;
}



XA_FILE *rsc_lang_file_open(const char *name)
{
	rsl_errors = 0;
	rsl_lno = 0;
	reported_skipped = 0;
	BLOG((0, "open lang-file '%s'", name));
	return xa_fopen(name, O_RDONLY);
}


XA_FILE *rsc_lang_file_create(const char *name)
{
	return xa_fopen(name, O_WRONLY | O_CREAT | O_TRUNC);
}


void rsc_lang_file_write(XA_FILE *fp, const char *buf, long l)
{
	if (fp)
	{
		xa_writeline(buf, l, fp);
	}
}


void rsc_lang_translate_object(struct xa_client *client, XA_FILE *rfp, OBJECT *obj, bool *strip)
{
	char **p = 0;
	short blen;

	if (obj->ob_flags & OF_NOTRANS)
		return;
	if (object_has_tedinfo(obj))
	{
		TEDINFO *ted = object_get_tedinfo(obj, NULL);

		if (ted)
		{
			if ((obj->ob_type & 0xff) == G_FTEXT || (obj->ob_type & 0xff) == G_FBOXTEXT)
				p = &ted->te_ptmplt;
			else
				p = &ted->te_ptext;
		}
	} else
	{
		p = &obj->ob_spec.free_string;
	}

	if (!p || !*p || !**p || strchr(*p, '\r') != 0 || strchr(*p, '\n') != 0)
		return;

	blen = rsc_trans_rw(client, rfp, p, strip);

	if (blen)
	{
		if (client->options.rsc_lang == READ)
		{
			if (blen > 0)				/* orig and trans found */
			{
				if (obj->ob_type == G_BUTTON && (obj->ob_flags & OF_SELECTABLE))
				{
					obj->ob_state &= 0x80ff;	/* make own shortcuts */
				}
			} else
			{
				blen = -blen;			/* only orig found */
			}
		}
	}
}


void rsc_lang_fix_menu(struct xa_client *client, OBJECT *menu, XA_FILE *rfp)
{
	short the_bar;
	short the_active;
	short title, menubox, index;
	short x;
	bool strip, tstrip;
	
	the_bar = menu[ROOT].ob_head;
	if (the_bar < 0 || (menu[the_bar].ob_type & 0xff) != G_BOX)
		return;
	the_active = menu[the_bar].ob_head;
	if (the_active < 0 || (menu[the_active].ob_type & 0xff) != G_IBOX)
		return;
	menubox = menu[menu[ROOT].ob_tail].ob_head;
	if (menubox < 0 || (menu[menubox].ob_type & 0xff) != G_BOX)
		return;
	title = menu[the_active].ob_head;
	if (title < 0 || (menu[title].ob_type & 0xff) != G_TITLE)
		return;
	x = menu[title].ob_x;
	do
	{
		char *str;
		short l;
		short maxw;
		short keep_w;
		
		tstrip = true;
		switch (menu[title].ob_type & 0xff)
		{
		case G_TEXT:
		case G_BOXTEXT:
		case G_FTEXT:
		case G_FBOXTEXT:
		case G_BUTTON:
		case G_STRING:
		case G_SHORTCUT:
		case G_TITLE:
			if (client->options.rsc_lang)
			{
				rsc_lang_translate_object(client, rfp, &menu[title], &tstrip);
			}
			break;
		}
		if ((menu[title].ob_type & 0xff) == G_TITLE)
		{
			str = menu[title].ob_spec.free_string;
			l = (short)strlen(str);
			while (l > 0 && str[l - 1] == ' ')
				l--;
			l++;
			menu[title].ob_width = l;
		}
		menu[title].ob_x = x;
		menu[menubox].ob_x = x + menu[the_active].ob_x;
		index = menu[menubox].ob_head;
		maxw = 0;
		keep_w = 0;
		do
		{
			switch (menu[index].ob_type & 0xff)
			{
			case G_TEXT:
			case G_BOXTEXT:
			case G_FTEXT:
			case G_FBOXTEXT:
			case G_BUTTON:
			case G_STRING:
			case G_SHORTCUT:
			case G_TITLE:
				if (client->options.rsc_lang)
				{
					strip = tstrip;
					rsc_lang_translate_object(client, rfp, &menu[index], &strip);
				}
				break;
			}
			if ((menu[index].ob_type & 0xff) == G_STRING)
			{
				str = menu[index].ob_spec.free_string;
				if (*str != '-')
				{
					l = (short)strlen(str);
					while (l > 0 && str[l - 1] == ' ')
						l--;
					l++;
					if (l > maxw)
						maxw = l;
					if (menu[index].ob_x > 0)
						keep_w = 1;	/* more than one obj in one line -> don't adjust width */
				}
			}
			index = menu[index].ob_next;
		} while (index != menubox);
		if (!keep_w)
		{
			menu[menubox].ob_width = maxw;
			index = menu[menubox].ob_head;
			do
			{
				menu[index].ob_width = maxw;
				index = menu[index].ob_next;
			} while (index != menubox);
		}
		
		menubox = menu[menubox].ob_next;

		x += menu[title].ob_width;
		title = menu[title].ob_next;
	} while (title != the_active);
}
