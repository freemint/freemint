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

#include "fix_rsc.h"

struct foobar
{
	short dummy;	/* ist wahrscheinlich vorhanden, weil irgendein Kretin
			   zwei Bytes zuviel rausgeschrieben hat */
	_WORD *image;	/* Zeiger auf Image */
};

static void fix_trindex(short num_tree, OBJECT **rs_trindex, OBJECT *rs_object);
static void fix_objects(short num_objs, OBJECT *obj, TEDINFO *ted, char *str[],
			ICONBLK *icon, BITBLK *bblk, struct foobar *rs_imdope);
static void fix_str(char **where, char *str[]);
static void fix_img(_WORD **where, struct foobar *rs_imdope);
static void fix_frstr(short num_frstr, long *rs_frstr, char *str[]);
static void fix_frimg(short num_frimg, BITBLK **rs_frimg, BITBLK *bblk,
		      struct foobar *rs_imdope);

/*----------------------------------------------------------------------------------------*/ 
/* Indizes in OBJECT-Strukturen in Zeiger umwandeln */
/* Funktionsresultat:	- */
/*	num_objs:	Anzahl aller Objekte in rs_object */
/*	num_frstr:	Anzahl der freien Strings in rs_frstr */
/*	num_frimg:	Anzahl der freien BITBLKs in rs_frimg */
/*	num_tree:	Anzahl der Baeume in rs_trindex */
/*	rs_object:	Feld mit saemtlichen Resource-OBJECTs */
/*	rs_tedinfo:	Feld mit den von OBJECTs angesprochenen TEDINFOs */
/*	rs_string:	Feld mit Strings */
/*	rs_iconblk:	Feld mit ICONBLKs fuer OBJECTs */
/*	rs_bitblk:	Feld mit BITBLKs fuer OBJECTs */
/*	rs_frstr:	Feld mit Indizes/Zeigern auf freie Strings */
/*	rs_frimg:	Feld mit Indizes/Zeigern auf freie Images */
/*	rs_trindex:	Feld mit Indizes/Zeigern auf Objektbaeume */
/*	rs_imdope:	Feld mit Zeigern auf Images fr ICONBLKs */
/*----------------------------------------------------------------------------------------*/ 
void
fix_rsc(short num_objs, short num_frstr, short num_frimg, short num_tree,
	OBJECT *rs_object, TEDINFO *rs_tedinfo, char *rs_string[], ICONBLK *rs_iconblk, BITBLK *rs_bitblk,
	long *rs_frstr, long *rs_frimg, long *rs_trindex, struct foobar *rs_imdope)
{
	fix_trindex(num_tree, (OBJECT **) rs_trindex, rs_object);
	fix_objects(num_objs, rs_object, rs_tedinfo, rs_string, rs_iconblk, rs_bitblk, rs_imdope);
	fix_frstr(num_frstr, rs_frstr, rs_string);
	fix_frimg(num_frimg, (BITBLK **) rs_frimg, rs_bitblk, rs_imdope);
}

/*----------------------------------------------------------------------------------------*/ 
/* Indizes rs_trindex durch Zeiger auf die Objektbaeume ersetzen */
/* Funktionsresultat:	- */
/*	num_tree:	Anzahl der Baeume in rs_trindex */
/*	rs_trindex:	Feld mit Indizes/Zeigern auf Objektbaeume */
/*	rs_object:	Feld mit saemtlichen Resource-OBJECTs */
/*----------------------------------------------------------------------------------------*/ 
static void
fix_trindex(short num_tree, OBJECT **rs_trindex, OBJECT *rs_object)
{
	while (num_tree > 0)
	{
		long index;

		index = (long) *rs_trindex; /* Wurzelobjektnummer */
		*rs_trindex = &rs_object[index]; /* Zeiger auf das Wurzelobjekt */
		rs_trindex++;
		num_tree--;
	}
}

/*----------------------------------------------------------------------------------------*/ 
/* Fr alle OBJECTs ob_spec und dessen Referenzen initialisieren */
/* Funktionsresultat:	- */
/*	num_objs:	Anzahl aller Objekte in rs_object */
/*	obj:		Feld mit saemtlichen Resource-OBJECTs */
/*	ted:		Feld mit den von OBJECTs angesprochenen TEDINFOs */
/*	str:		Feld mit Strings */
/*	icon:		Feld mit ICONBLKs fuer OBJECTs */
/*	bblk:		Feld mit BITBLKs fuer OBJECTs */
/*	rs_imdope:	Feld mit Zeigern auf Images fuer ICONBLKs */
/*----------------------------------------------------------------------------------------*/ 
static void
fix_objects(short num_objs, OBJECT *obj, TEDINFO *ted, char *str[],
	    ICONBLK *icon, BITBLK *bblk, struct foobar *rs_imdope)
{
	while (num_objs > 0)
	{
		long index = obj->ob_spec.index;

		switch (obj->ob_type & 0xff)
		{
			case G_TITLE:
			case G_STRING:
			case G_BUTTON:
			{
				fix_str(&obj->ob_spec.free_string, str); /* Zeiger auf String */
				break;
			}
			case G_TEXT:
			case G_BOXTEXT:
			case G_FTEXT:
			case G_FBOXTEXT:
			{
				if (index != -1L) /* TEDINFO besetzen? */
				{
					obj->ob_spec.tedinfo = &ted[index];
					fix_str(&ted[index].te_ptext, str); /* Zeiger auf String */
					fix_str(&ted[index].te_ptmplt, str); /* Zeiger auf String */
					fix_str(&ted[index].te_pvalid, str); /* Zeiger auf String */
				}
				else
					obj->ob_spec.tedinfo = 0L;

				break;
			}
			case G_ICON:
			{
				if (index != -1L) /* ICONBLK besetzen? */
				{
					obj->ob_spec.iconblk = &icon[index]; /* Zeiger auf ICONBLK */
					fix_img(&icon[index].ib_pmask, rs_imdope); /* Zeiger auf die Maske */
					fix_img(&icon[index].ib_pdata, rs_imdope); /* Zeiger auf den Vordergrund */
					fix_str(&icon[index].ib_ptext, str); /* Zeiger auf String */
				}
				else
					obj->ob_spec.iconblk = 0L;

				break;
			}
			case G_IMAGE:
			{
				if (index != -1L) /* BITBLK besetzen? */
				{
					obj->ob_spec.bitblk = &bblk[index];
					fix_img(&bblk[index].bi_pdata, rs_imdope);
				}
				else
					obj->ob_spec.bitblk = 0L;

				break;
			}
		}
		obj++;
		num_objs--;
	}
}

/*----------------------------------------------------------------------------------------*/ 
/* Stringadresse eintragen */
/* Funktionsresultat:	- */
/*	where:		Adresse des Index/Zeigers */
/*	str:		Feld mit Strings */
/*----------------------------------------------------------------------------------------*/ 
static void
fix_str(char **where, char *str[])
{
	long index = (long) *where;

	if (index != -1L) /* String vorhanden? */
		*where = str[index];
	else
		*where = 0L;
}

/*----------------------------------------------------------------------------------------*/ 
/* Zeiger auf Image eintragen */
/* Funktionsresultat:	- */
/*	where:		Adresse des Index/Zeigers */
/*	rs_imdope:	Feld mit Zeigern auf Images fuer ICONBLKs */
/*----------------------------------------------------------------------------------------*/ 
static void
fix_img(_WORD **where, struct foobar *rs_imdope)
{
	long index = (long) *where;

	if (index != -1L) /* Image vorhanden? */
		*where = rs_imdope[index].image;
	else
		*where = 0L;
}

/*----------------------------------------------------------------------------------------*/ 
/* freie Strings verzeigern */
/* Funktionsresultat:	- */
/*	num_frstr:	Anzahl der freien Strings in rs_frstr */
/*	rs_frstr:	Feld mit Indizes/Zeigern auf freie Strings */
/*	str:		Feld mit Strings */
/*----------------------------------------------------------------------------------------*/ 
static void
fix_frstr(short num_frstr, long *rs_frstr, char *str[])
{
	while (num_frstr > 0)
	{
		fix_str((char **)rs_frstr, str);
		rs_frstr++;
		num_frstr--;
	}
}

/*----------------------------------------------------------------------------------------*/ 
/* freie BITBLKs verzeigern */
/* Funktionsresultat:	- */
/*	num_frimg:	Anzahl der freien BITBLKs in rs_frimg */
/*	rs_frimg:	Feld mit Indizes/Zeigern auf freie Images */
/*	bblk:		Feld mit BITBLKs fuer OBJECTs */
/*	rs_imdope:	Feld mit Zeigern auf Images fuer ICONBLKs */
/*----------------------------------------------------------------------------------------*/ 
static void
fix_frimg(short num_frimg, BITBLK **rs_frimg, BITBLK *bblk, struct foobar *rs_imdope)
{
	while (num_frimg > 0)
	{
		long index = (long) *rs_frimg;

		if (index != -1L) /* BITBLK vorhanden? */
		{
			*rs_frimg = &bblk[index];
			fix_img(&bblk[index].bi_pdata, rs_imdope);
		}
		else
			*rs_frimg = 0L;

		rs_frimg++;
		num_frimg--;
	}
}
