/*
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

/*
	Tabulatorweite: 8
	Kommentare ab: Spalte 			*Spalte 50*
*/
#ifndef _DRAGDROP_H_
#define _DRAGDROP_H_

#ifndef AP_DRAGDROP
#define	AP_DRAGDROP	63
#endif

#define	DD_OK		0
#define	DD_NAK		1
#define	DD_EXT		2
#define	DD_LEN		3

#define	DD_TIMEOUT	4000				/* Timeout in ms */

#define	DD_NUMEXTS	8				/* Anzahl der Formate */
#define	DD_EXTSIZE	32				/* Laenge des Formatfelds */


#define	DD_FNAME	"u:\\pipe\\dragdrop.aa"
#define	DD_NAMEMAX	128				/* maximale Laenge eines Formatnamens */
#define	DD_HDRMIN	9				/* minmale Laenge des Drag&Drop-Headers */
#define	DD_HDRMAX	( 8 + DD_NAMEMAX )		/* maximale Laenge des Drag&Drop-Headers */

short	ddcreate( short	app_id, short rcvr_id, short window, short mx, short my, short kbstate, unsigned long format[8], void **oldpipesig );
short	ddstry( short handle, unsigned long format, char *name, long size );
short	ddopen( char *pipe, unsigned long *format, void **oldpipesig );
void	ddclose( short handle, void *oldpipesig );
short	ddrtry( short handle, char *name, unsigned long *format, long *size );
short	ddreply( short handle, short msg );

#endif
