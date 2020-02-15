/*
 * COPS (c) 1995 - 2003 Sven & Wilfried Behne
 *                 2004 F.Naumann & O.Skancke
 *
 * A XCONTROL compatible CPX manager.
 *
 * This file is part of COPS.
 *
 * COPS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * COPS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <string.h>
#include <stdio.h>

#include <mint/cookie.h>

#include "cops_rsc.h"
#include "adaptrsc.h"
#include "cops.h"


/* XXX */
#ifndef OF_FL3DMASK
#define OF_FL3DMASK	0x0600
#endif
#ifndef OF_FL3DIND
#define OF_FL3DIND	0x0200
#endif
#ifndef AD3DVALUE
#define AD3DVALUE	6
#endif


#ifndef _AESVARS
#define _AESVARS
typedef struct
{
	long	magic;						/* must be 0x87654321 */
	void	*membot;					/* Ende der AES-Variablen */
	void	*aes_start;					/* Startadresse */

	long	magic2;						/* ist 'MAGX' */
	long	date;						/* Erstelldatum: ttmmjjjj */
	void	(*chgres)(short res, short txt);	/* Aufloesung aendern */
	long	(**shel_vector)(void);		/* ROM-Desktop */
	char	*aes_bootdrv;				/* Hierhin kommt DESKTOP.INF */
	short	*vdi_device;				/* vom AES benutzter Treiber */
	void	*reservd1;
	void	*reservd2;
	void	*reservd3;

	short	version;					/* Versionsnummer */
	short	release;					/* Release-Status */

} AESVARS;
#endif

#ifndef _MAGX_COOKIE
#define _MAGX_COOKIE
typedef struct
{
	long	config_status;
	void	*dosvars;
	AESVARS	*aesvars;
} MAGX_COOKIE;
#endif


static USERBLK *substitute_ublks;
static OBJECT *radio_slct;
static OBJECT *radio_deslct;
static _WORD radio_bgcol;
static _WORD magic_version;

static _WORD _cdecl check_button(PARMBLK *parmblock);
static _WORD _cdecl radio_button(PARMBLK *parmblock);
static _WORD _cdecl group(PARMBLK *parmblock);
static _WORD _cdecl title(PARMBLK *parmblock);
static void userdef_text(_WORD x, _WORD y, char *string);

/*----------------------------------------------------------------------------------------*/
/* Informationen ueber die AES-Funktionen zurueckliefern                                  */
/* Funktionsergebnis:   diverse Flags                                                     */
/*  font_id:        ID des AES-Fonts                                                      */
/*  font_height:    Hoehe des AES-Fonts (fuer vst_height())                               */
/*  hor_3d:         zusaetzlicher horizontaler beidseitiger Rand fuer 3D-Objekte          */
/*  ver_3d:         zusaetzlicher vertikaler beidseitiger Rand fuer 3D-Objekte            */
/*----------------------------------------------------------------------------------------*/
_WORD get_aes_info(_WORD *font_id, _WORD *font_height, _WORD *hor_3d, _WORD *ver_3d)
{
	MAGX_COOKIE *magic;
	_WORD work_out[57];
	_WORD attrib[10];
	_WORD pens;
	_WORD flags;
	
	vq_extnd(vdi_handle, 0, work_out);
	vqt_attributes(vdi_handle, attrib);

	flags = 0;
	pens = work_out[13];		/* Anzahl der Farbstifte */
	*font_id = attrib[0];		/* Standardfont */
	*font_height = attrib[7];	/* Standardh”he */
	*hor_3d = 0;
	*ver_3d = 0;
	radio_bgcol = 0;

	if (appl_find("?AGI") == 0)	/* appl_getinfo() vorhanden? */
		flags |= GAI_INFO;

	if (_AESversion >= 0x0401)	/* mindestens AES 4.01? */
		flags |= GAI_INFO;

	if (Getcookie(C_MagX, (long *)(&magic)) != 0)
		magic = NULL;
	magic_version = 0;

	if (magic)			/* MagiC vorhanden? */
	{
		if (magic->aesvars)	/* MagiC-AES aktiv? */
		{
			magic_version = magic->aesvars->version; /* MagiC-Versionsnummer */
			flags |= GAI_MAGIC + GAI_INFO;
		}
	}

	if (flags & GAI_INFO)				/* ist appl_getinfo() vorhanden? */
	{
		_WORD ag1;
		_WORD ag2;
		_WORD ag3;
		_WORD ag4;

		if (appl_getinfo(0, &ag1, &ag2, &ag3, &ag4))		/* Unterfunktion 0, Fonts */
		{
			*font_id = ag2;
			*font_height = ag1;
		}

		if (appl_getinfo(2, &ag1, &ag2, &ag3, &ag4) && ag3)	/* Unterfunktion 2, Farben */
			flags |= GAI_CICN;

		if (appl_getinfo(7, &ag1, &ag2, &ag3, &ag4))		/* Unterfunktion 7 */
			flags |= ag1 & (GAI_WDLG|GAI_LBOX|GAI_FNTS|GAI_FSEL);

		if (appl_getinfo(12, &ag1, &ag2, &ag3, &ag4) && (ag1 & 8)) /* AP_TERM? */
			flags |= GAI_APTERM;

		if (appl_getinfo(13, &ag1, &ag2, &ag3, &ag4))		/* Unterfunktion 13, Objekte */
		{
			if (flags & GAI_MAGIC) /* MagiC spezifische Funktion! */
			{
				if (ag4 & 0x08) /* G_SHORTCUT untersttzt ? */
					flags |= GAI_GSHORTCUT;
			}
				
			if (ag1 && ag2)									/* 3D-Objekte und objc_sysvar() vorhanden? */
			{
				if (objc_sysvar(0, AD3DVALUE, 0, 0, hor_3d, ver_3d))	/* 3D-Look eingeschaltet? */
				{
					if (pens >= 16)							/* mindestens 16 Farben? */
					{
						_WORD dummy;
						
						flags |= GAI_3D;
						objc_sysvar(0, BACKGRCOL, 0, 0, &radio_bgcol, &dummy);
					}
				}
			}
		}
	}

	return flags;
}

/*----------------------------------------------------------------------------------------*/
/* Horizontale und Vertikale Vergroesserung und Verschiebung von 3D-Objekten kompensieren */
/* Funktionsergebnis:   -                                                                 */
/* objs:        Zeiger auf die Objekte                                                    */
/*  no_objs:    Anzahl der Objekte                                                        */
/*  hor_3d:     zusaetzlicher horizontaler beidseitiger Rand fuer 3D-Objekte              */
/*  ver_3d:     zusaetzlicher vertikaler beidseitiger Rand fuer 3D-Objekte                */
/*----------------------------------------------------------------------------------------*/
void adapt3d_rsrc(OBJECT *objs, unsigned short no_objs, _WORD hor_3d, _WORD ver_3d)
{
	while (no_objs > 0)
	{
		if (objs->ob_flags & OF_FL3DIND)	/* Indicator oder Activator? */
		{
			objs->ob_x += hor_3d;
			objs->ob_y += ver_3d;
			objs->ob_width -= 2 * hor_3d;
			objs->ob_height -= 2 * ver_3d;
		}
		objs++;							/* naechstes Objekt */
		no_objs--;
	}
}

/*----------------------------------------------------------------------------------------*/
/* 3D-Flags loeschen und Objektgroessen anpassen, wenn 3D-Look ausgeschaltet ist          */
/* Funktionsergebnis:   -                                                                 */
/* objs:            Zeiger auf die Objekte                                                */
/*  no_objs:        Anzahl der Objekte                                                    */
/*  ftext_to_fboxtext:  Flag dafuer, dass FTEXT-Objekte in FBOXTEXT umgewandelt werden    */
/*----------------------------------------------------------------------------------------*/
void no3d_rsrc(OBJECT *objs, unsigned short no_objs, short ftext_to_fboxtext)
{
	radio_bgcol = 0; /* Annahme: Hintergrundfarbe bei 2D ist weiss */
	
	while (no_objs > 0)
	{
		if (ftext_to_fboxtext)			/* FTEXT-Objekte in FBOXTEXT umwandeln? */
		{
			if ((objs->ob_type & 0xff) == G_FTEXT)	/* FTEXT-Objekt? */
			{
				if (objs->ob_flags & OF_FL3DMASK)	/* mit 3D-Optik? */
				{
					if (objs->ob_spec.tedinfo->te_thickness == -2)
					{
						objs->ob_state |= OS_OUTLINED;
						objs->ob_spec.tedinfo->te_thickness = -1;
						objs->ob_type = G_FBOXTEXT;
					}
				}
			}
		}

		objs->ob_flags &= ~OF_FL3DMASK;	/* 3D-Flags loeschen */

		objs++;							/* naechstes Objekt */
		no_objs--;
	}
}

/*----------------------------------------------------------------------------------------*/
/* Testen, ob ein Objekt USERDEF ist und ob es einen G_STRING-Titel ersetzt               */
/* Funktionsergebnis:   Zeiger auf den String oder NULL, wenn es kein USERDEF-Titel ist   */
/*  obj:        Zeiger auf das Objekt                                                     */
/*----------------------------------------------------------------------------------------*/
char *is_userdef_title(OBJECT *obj)
{
	if (substitute_ublks)				/* werden MagiC-Objekte ersetzt? */
	{
		if (obj->ob_type == G_USERDEF)
		{
			USERBLK *ublk;

			ublk = obj->ob_spec.userblk;

			if (ublk->ub_code == title)	/* ersetzte Ueberschrift? */
				return ((char *) ublk->ub_parm);	/* Zeiger auf den String zurueckliefern */
		}
	}
	return NULL;
}

/*----------------------------------------------------------------------------------------*/
/* MagiC-Objekte durch USERDEFs ersetzen                                                  */
/* Funktionsergebnis: -                                                                   */
/* objs:        Zeiger auf die Objekte                                                    */
/*  no_objs:    Anzahl der Objekte                                                        */
/*  aesflags:   Informationen ueber das AES                                               */
/*  rslct:      Zeiger auf Image fuer selektierten Radio-Button                           */
/*  rdeslct:    Zeiger auf Image fuer deselektierten Radio-Button                         */
/*----------------------------------------------------------------------------------------*/
void substitute_objects(OBJECT *objs, unsigned short no_objs, short aesflags, OBJECT *rslct, OBJECT *rdeslct)
{
	OBJECT *obj;
	unsigned short i;
	unsigned short no_subs;

	if ((aesflags & GAI_MAGIC) && (magic_version >= 0x0300))	/* MagiC-AES? */
	{
		substitute_ublks = 0L;
		return;
	}

	obj = objs;							/* Zeiger auf die Objekte */
	i = no_objs;						/* Anzahl der Objekte im gesamten Resource */

	no_subs = 0;

	while (i > 0)
	{
		if ((obj->ob_state & OS_WHITEBAK) && (obj->ob_state & 0x8000))	/* MagiC-Objekt? */
		{
			switch (obj->ob_type & 0xff)
			{
			/* Checkbox, Radiobutton oder Gruppenrahmen */
			case G_BUTTON:
				no_subs++;
				break;
			/* Ueberschrift */
			case G_STRING:
				if ((obj->ob_state & 0xff00) == 0xff00)		/* Unterstreichung auf voller Laenge? */
					no_subs++;
				break;
			}
		}
		obj++;							/* naechstes Objekt */
		i--;
	}

	if (no_subs)						/* sind MagiC-Objekte vorhanden? */
	{
		substitute_ublks = (void *)Malloc(no_subs * sizeof(USERBLK));
		radio_slct = rslct;
		radio_deslct = rdeslct;

		if (substitute_ublks)			/* Speicher vorhanden? */
		{
			USERBLK *tmp;

			tmp = substitute_ublks;

			obj = objs;					/* Zeiger auf die Objekte */
			i = no_objs;				/* Anzahl der Objekte im gesamten Resource */

			while (i > 0)
			{
				short type;
				unsigned short state;

				type = obj->ob_type & 0x00ff;
				state = obj->ob_state;

				if ((state & OS_WHITEBAK) && (state & 0x8000))	/* MagiC-Objekt? */
				{
					state &= 0xff00;	/* nur das obere Byte ist interessant */

					if (aesflags & GAI_MAGIC)	/* altes MagiC-AES? */
					{
						if ((type == G_BUTTON) && (state == 0xfe00))	/* Gruppenrahmen? */
						{
							tmp->ub_parm = (long) obj->ob_spec.free_string;	/* Zeiger auf den Text */
							tmp->ub_code = group;

							obj->ob_type = G_USERDEF;
							obj->ob_flags &= ~OF_FL3DMASK;		/* 3D-Flags loeschen */
							obj->ob_spec.userblk = tmp;			/* Zeiger auf den USERBLK */

							tmp++;
						}
					} else				/* TOS-AES oder sonstiges */
					{
						switch (type)
						{
						/* Checkbox, Radiobutton oder Gruppenrahmen */
						case G_BUTTON:
							/* Zeiger auf den Text */
							tmp->ub_parm = (long) obj->ob_spec.free_string;

							if (state == 0xfe00)	/* Gruppenrahmen? */
								tmp->ub_code = group;
							else if (obj->ob_flags & OF_RBUTTON)	/* Radio-Button? */
								tmp->ub_code = radio_button;
							else					/* Check-Button */
								tmp->ub_code = check_button;

							obj->ob_type = G_USERDEF;
							obj->ob_flags &= ~OF_FL3DMASK;			/* 3D-Flags loeschen */
							obj->ob_spec.userblk = tmp;				/* Zeiger auf den USERBLK */

							tmp++;
							break;

						/* Ueberschrift */
						case G_STRING:
							if (state == 0xff00)					/* Unterstreichung auf voller L„nge? */
							{
								/* Zeiger auf den Text */
								tmp->ub_parm = (long) obj->ob_spec.free_string;
								tmp->ub_code = title;
								obj->ob_type = G_USERDEF;
								obj->ob_flags &= ~OF_FL3DMASK;		/* 3D-Flags loeschen */
								obj->ob_spec.userblk = tmp;			/* Zeiger auf den USERBLK */
								tmp++;
							}
							break;
						}
					}
				}
				obj++;					/* naechstes Objekt */
				i--;
			}
		}
	}
}

/*----------------------------------------------------------------------------------------*/
/* Speicher fuer Resource-Anpassung freigeben                                             */
/* Funktionsresultat:   -                                                                 */
/*----------------------------------------------------------------------------------------*/
void substitute_free(void)
{
	if (substitute_ublks)				/* Speicher vorhanden? */
		Mfree(substitute_ublks);

	substitute_ublks = NULL;
}

/*----------------------------------------------------------------------------------------*/
/* USERDEF-Funktion fuer Check-Button                                                     */
/* Funktionsresultat:   nicht aktualisierte Objektstati                                   */
/* parmblock:       Zeiger auf die Parameter-Block-Struktur                               */
/*----------------------------------------------------------------------------------------*/
static _WORD _cdecl check_button(PARMBLK *parmblock)
{
	_WORD rect[4];
	_WORD clip[4];
	_WORD xy[10];
	char *string;

	string = (char *) parmblock->pb_parm;

	/* clipping rectangle... */
	clip[0] = parmblock->pb_xc;
	clip[1] = parmblock->pb_yc;
	clip[2] = parmblock->pb_xc + parmblock->pb_wc - 1;
	clip[3] = parmblock->pb_yc + parmblock->pb_hc - 1;
	udef_vs_clip(vdi_handle, 1, clip);				/* Zeichenoperationen auf gegebenen Bereich beschraenken */

	/* object rectangle */
	rect[0] = parmblock->pb_x;
	rect[1] = parmblock->pb_y;
	rect[2] = rect[0] + phchar - 2;
	rect[3] = rect[1] + phchar - 2;

	udef_vswr_mode(vdi_handle, 1);		/* Ersetzend */

	udef_vsl_color(vdi_handle, G_BLACK);

	xy[0] = rect[0];
	xy[1] = rect[1];
	xy[2] = rect[2];
	xy[3] = rect[1];
	xy[4] = rect[2];
	xy[5] = rect[3];
	xy[6] = rect[0];
	xy[7] = rect[3];
	xy[8] = rect[0];
	xy[9] = rect[1];
	udef_v_pline(vdi_handle, 5, xy);	/* schwarzen Rahmen zeichnen */

	udef_vsf_color(vdi_handle, G_WHITE);

	xy[0] = rect[0] + 1;
	xy[1] = rect[1] + 1;
	xy[2] = rect[2] - 1;
	xy[3] = rect[3] - 1;
	udef_vr_recfl(vdi_handle, xy);		/* weisse Box zeichnen */

	if (parmblock->pb_currstate & OS_SELECTED)
	{
		parmblock->pb_currstate &= ~OS_SELECTED;	/* Bit loeschen */

		udef_vsl_color(vdi_handle, G_BLACK);		/* schwarz - fuer das Kreuz */
		xy[0] = rect[0] + 2;
		xy[1] = rect[1] + 2;
		xy[2] = rect[2] - 2;
		xy[3] = rect[3] - 2;
		udef_v_pline(vdi_handle, 2, xy);

		xy[1] = rect[3] - 2;
		xy[3] = rect[1] + 2;
		udef_v_pline(vdi_handle, 2, xy);
	}
	userdef_text(parmblock->pb_x + phchar + pwchar, parmblock->pb_y, string);

	return parmblock->pb_currstate;
}

/*----------------------------------------------------------------------------------------*/
/* USERDEF-Funktion fuer Radio-Button                                                     */
/* Funktionsresultat:   nicht aktualisierte Objektstati                                   */
/* parmblock:       Zeiger auf die Parameter-Block-Struktur                               */
/*----------------------------------------------------------------------------------------*/
static _WORD _cdecl radio_button(PARMBLK *parmblock)
{
	BITBLK *image;
	MFDB src;
	MFDB des;
	_WORD clip[4];
	_WORD xy[8];
	_WORD image_colors[2];
	char *string;

	/* clipping rectangle... */
	clip[0] = parmblock->pb_xc;
	clip[1] = parmblock->pb_yc;
	clip[2] = parmblock->pb_xc + parmblock->pb_wc - 1;
	clip[3] = parmblock->pb_yc + parmblock->pb_hc - 1;
	udef_vs_clip(vdi_handle, 1, clip);				/* Zeichenoperationen auf gegebenen Bereich beschr„nken */

	string = (char *) parmblock->pb_parm;

	if (parmblock->pb_currstate & OS_SELECTED)		/* Selektion? */
	{
		parmblock->pb_currstate &= ~OS_SELECTED;	/* Bit loeschen */
		image = radio_slct->ob_spec.bitblk;
	} else
	{
		image = radio_deslct->ob_spec.bitblk;
	}

	src.fd_addr = image->bi_pdata;
	src.fd_w = image->bi_wb * 8;
	src.fd_h = image->bi_hl;
	src.fd_wdwidth = image->bi_wb / 2;
	src.fd_stand = 0;
	src.fd_nplanes = 1;
	src.fd_r1 = 0;
	src.fd_r2 = 0;
	src.fd_r3 = 0;

	des.fd_addr = 0L;

	xy[0] = 0;
	xy[1] = 0;
	xy[2] = src.fd_w - 1;
	xy[3] = src.fd_h - 1;
	xy[4] = parmblock->pb_x;
	xy[5] = parmblock->pb_y;
	xy[6] = xy[4] + xy[2];
	xy[7] = xy[5] + xy[3];

	image_colors[0] = G_BLACK;			/* schwarz als Vordergrundfarbe */
	image_colors[1] = radio_bgcol;		/* Hintergrundfarbe */

	udef_vrt_cpyfm(vdi_handle, MD_REPLACE, xy, &src, &des, image_colors);
	userdef_text(parmblock->pb_x + phchar + pwchar, parmblock->pb_y, string);

	return parmblock->pb_currstate;
}

/*----------------------------------------------------------------------------------------*/
/* USERDEF-Funktion fuer Gruppen-Rahmen                                                   */
/* Funktionsresultat:   nicht aktualisierte Objektstati                                   */
/* parmblock:       Zeiger auf die Parameter-Block-Struktur                               */
/*----------------------------------------------------------------------------------------*/
static _WORD _cdecl group(PARMBLK *parmblock)
{
	_WORD clip[4];
	_WORD obj[4];
	_WORD xy[12];
	char *string;

	string = (char *) parmblock->pb_parm;

	/* clipping rectangle... */
	clip[0] = parmblock->pb_xc;
	clip[1] = parmblock->pb_yc;
	clip[2] = parmblock->pb_xc + parmblock->pb_wc - 1;
	clip[3] = parmblock->pb_yc + parmblock->pb_hc - 1;
	udef_vs_clip(vdi_handle, 1, clip);				/* Zeichenoperationen auf gegebenen Bereich beschraenken */

	udef_vswr_mode(vdi_handle, MD_TRANS);
	udef_vsl_color(vdi_handle, G_BLACK);
	udef_vsl_type(vdi_handle, 1);

	/* object rectangle */
	obj[0] = parmblock->pb_x;
	obj[1] = parmblock->pb_y;
	obj[2] = obj[0] + parmblock->pb_w - 1;
	obj[3] = obj[1] + parmblock->pb_h - 1;

	xy[0] = obj[0] + pwchar;
	xy[1] = obj[1] + phchar / 2;
	xy[2] = obj[0];
	xy[3] = xy[1];
	xy[4] = obj[0];
	xy[5] = obj[3];
	xy[6] = obj[2];
	xy[7] = obj[3];
	xy[8] = obj[2];
	xy[9] = xy[1];
	xy[10] = (short) (xy[0] + strlen(string) * pwchar);
	xy[11] = xy[1];

	udef_v_pline(vdi_handle, 6, xy);

	userdef_text(obj[0] + pwchar, obj[1], string);

	return parmblock->pb_currstate;
}

/*----------------------------------------------------------------------------------------*/
/* USERDEF-Funktion fuer Ueberschrift                                                     */
/* Funktionsresultat:   nicht aktualisierte Objektstati                                   */
/* parmblock:       Zeiger auf die Parameter-Block-Struktur                               */
/*----------------------------------------------------------------------------------------*/
static _WORD _cdecl title(PARMBLK *parmblock)
{
	_WORD clip[4];
	_WORD xy[4];
	char *string;

	string = (char *) parmblock->pb_parm;

	/* clipping rectangle... */
	clip[0] = parmblock->pb_xc;
	clip[1] = parmblock->pb_yc;
	clip[2] = parmblock->pb_xc + parmblock->pb_wc - 1;
	clip[3] = parmblock->pb_yc + parmblock->pb_hc - 1;
	udef_vs_clip(vdi_handle, 1, clip);				/* Zeichenoperationen auf gegebenen Bereich beschraenken */

	udef_vswr_mode(vdi_handle, MD_TRANS);
	udef_vsl_color(vdi_handle, G_BLACK);
	udef_vsl_type(vdi_handle, 1);

	xy[0] = parmblock->pb_x;
	xy[1] = parmblock->pb_y + parmblock->pb_h - 1;
	xy[2] = parmblock->pb_x + parmblock->pb_w - 1;
	xy[3] = xy[1];
	udef_v_pline(vdi_handle, 2, xy);

	userdef_text(parmblock->pb_x, parmblock->pb_y, string);

	return parmblock->pb_currstate;
}

static void userdef_text(_WORD x, _WORD y, char *string)
{
	_WORD tmp;

	udef_vswr_mode(vdi_handle, MD_TRANS);
	udef_vst_font(vdi_handle, aes_font);		/* Font einstellen */
	udef_vst_color(vdi_handle, G_BLACK);		/* schwarz */
	udef_vst_effects(vdi_handle, 0);			/* keine Effekte */
	udef_vst_alignment(vdi_handle, 0, 5, &tmp, &tmp);	/* an der Zeichenzellenoberkante ausrichten */
	udef_vst_height(vdi_handle, aes_height, &tmp, &tmp, &tmp, &tmp);

	udef_v_gtext(vdi_handle, x, y, string);
}
