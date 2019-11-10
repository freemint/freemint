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
#include "xa_strings.h"

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
static short rsc_lang_file_replace(XA_FILE *fp, char **buf, short *chgl)
{
	char lbuf[256];
	char *p = 0;
	char *in = *buf;
	bool strip = true;
	int len = strlen(in);
	int found;
	short blen = 0;
	short clen;
	short cmplen;

	if (chgl && *chgl == 2)
	{
		strip = false;
	}

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
				if (chgl && *chgl == -1)
				{
					found = OFOUND;
					break;
				}
				if (strncmp(lbuf, "nn", 2) == 0)
				{
					if ((*p == '-' && *in == '-') || strncmp(p, in, cmplen) == 0)
					{
						found = OFOUND;
						if (lbuf[LF_OFFS - 1] == ';')	/* dont translate this */
							break;
						if (lbuf[LF_OFFS - 1] == '+')	/* realloc */
							strip = false;
					} else if (reported_skipped == 0)
					{
						BLOG((0, "%ld:'%s' skipped (expected:'%s')", rsl_lno, p, in));
						reported_skipped = 1;
					}
				}
			} else if (strncmp(lbuf, cfg.lang, 2) == 0)
			{
				/* copy shorter length */
				if (blen > len && strip)
					blen = len;
				found = TFOUND;
				break;					/* translation found */
			}
		}
	}

	if (found == TFOUND && p && lbuf[0] != -1 && lbuf[0] != LF_SEPCH)	/* found */
	{
		if (blen > len && strip == false)
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
	if (chgl && *chgl != 2)
	{
		*chgl = strip == false;
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


static short translate_string(struct xa_client *client, XA_FILE * rfp, char **p, short *chgl)
{
	short blen;
	short j;

	UNUSED(client);
	if (!p)
		return 0;

	for (blen = j = 0; j < 2 && !blen; j++)
	{
		if ((blen = rsc_lang_file_replace(rfp, p, chgl)))
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


short rsc_trans_rw(struct xa_client *client, XA_FILE *rfp, char **txt, short *chgl)
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
		blen = translate_string(client, rfp, txt, chgl);
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


void rsc_lang_translate_xaaes_strings(struct xa_client *client, XA_FILE *rfp)
{
	int i;
	char **t;

	if (trans_strings[0] == 0 || client->options.rsc_lang == 0)
		return;
	if (client->options.rsc_lang == WRITE)
	{
		rsc_lang_file_write(rfp, "# - Internal Strings -", 22);
	}
	for (i = 0; trans_strings[i]; i++)
	{
		for (t = trans_strings[i]; *t; t++)
		{
			if (**t)
			{
				char **t_2;
				short ptr_seen = 0;

				/* strings might be merged
				 * gcc4 puts all strings in the same order in the out-file as they are in the source
				 * gcc2 (and maybe also others) uses another order, so a search for the current string is done
				 */
				for (t_2 = trans_strings[i]; t_2 < t; t_2++)
					if (*t_2 == *t)
					{
						ptr_seen = -1;
						break;
					}

				rsc_trans_rw(client, rfp, t, &ptr_seen);
			}
		}
	}
	trans_strings[0] = 0;				/* no further translation possible */
}


void rsc_lang_translate_object(struct xa_client *client, XA_FILE *rfp, OBJECT *obj, struct rsc_scan *scan)
{
	char **p = 0;
	short blen;
	short chgl = 0;

	if (object_has_tedinfo(obj))
	{
		TEDINFO *ted = object_get_tedinfo(obj, NULL);

		if (ted)
		{
			p = &ted->te_ptext;			/* FIXME: must translate template, not text for FTEXT/FBOXTEXT */
		}
	} else
	{
		p = &obj->ob_spec.free_string;
	}

	if (!p || !*p || !**p || strchr(*p, '\r') != 0 || strchr(*p, '\n') != 0)
		return;

	if (client->options.rsc_lang == WRITE)
	{
		if (obj->ob_type == G_TITLE)
		{
			if (scan->n_titles == 0)
				rsc_lang_file_write(rfp, "### Menubar ###", 15);

			scan->titles[scan->n_titles++] = *p;
			scan->title = 0;
		} else if (obj->ob_type == G_STRING && scan->title > 0 && scan->titles[scan->title - 1])
		{
			char buf[256];
			int l = sprintf(buf, sizeof(buf) - 1, "### - \"%s\"-menu - ###", scan->titles[scan->title - 1]);

			rsc_lang_file_write(rfp, buf, l);
			scan->titles[scan->title - 1] = 0;
		}
	}

	blen = rsc_trans_rw(client, rfp, p,
			 obj->ob_type == G_TITLE ? &chgl :
			 scan->title > 0 ? &scan->change_lo[scan->title - 1] : 0);

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

			if (obj->ob_type == G_STRING)
			{
				if (**p != '-')
				{
					if (*(*p + blen - 1) != ' ')
						blen++;
					if (obj->ob_x > 0)
						scan->keep_w = 1;	/* more than one obj in one line -> don't adjust width */
					if (blen + obj->ob_x > scan->mwidth)
					{
						scan->mwidth = blen + obj->ob_x;
					}
				}
			} else if (obj->ob_type == G_TITLE && scan->n_titles >= 0)
			{
				short s;

				if (*(*p + blen - 1) != ' ')
					blen++;
				s = blen - obj->ob_width;
				scan->title = 0;
				if (scan->n_titles > 0)
					obj->ob_x += scan->shift[scan->n_titles - 1];

				if (s && chgl == true)
				{
					obj->ob_width = blen;
					scan->shift[scan->n_titles] = s;
				}
				scan->change_lo[scan->n_titles] = chgl * 2;

				if (scan->n_titles > 0 && scan->n_titles < MAX_TITLES)
				{
					scan->shift[scan->n_titles] += scan->shift[scan->n_titles - 1];
				}
				scan->n_titles++;
			}
		}
	}
}
