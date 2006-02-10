/*
	Tabulatorweite: 3
	Kommentare ab: Spalte 60											*Spalte 60*
*/
#ifndef _DRAGDROP_H_
#define _DRAGDROP_H_

#ifndef AP_DRAGDROP
#define	AP_DRAGDROP	63
#endif

#define	DD_OK		0
#define	DD_NAK	1
#define	DD_EXT	2
#define	DD_LEN	3

#define	DD_TIMEOUT	4000												/* Timeout in ms */

#define	DD_NUMEXTS	8													/* Anzahl der Formate */
#define	DD_EXTSIZE	32													/* Lnge des Formatfelds */


#define	DD_FNAME		"U:\\PIPE\\DRAGDROP.AA"
#define	DD_NAMEMAX	128												/* maximale Lnge eines Formatnamens */
#define	DD_HDRMIN	9													/* minmale Lnge des Drag&Drop-Headers */
#define	DD_HDRMAX	( 8 + DD_NAMEMAX )							/* maximale Lnge des Drag&Drop-Headers */

short	ddcreate( short	app_id, short rcvr_id, short window, short mx, short my, short kbstate, unsigned long format[8], void **oldpipesig );
short	ddstry( short handle, unsigned long format, char *name, long size );
short	ddopen( char *pipe, unsigned long *format, void **oldpipesig );
void	ddclose( short handle, void *oldpipesig );
short	ddrtry( short handle, char *name, unsigned long *format, long *size );
short	ddreply( short handle, short msg );

#endif
