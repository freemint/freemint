/*
 * $Id$
 * 
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

/********************************************
 *   begin options                          *
 ********************************************/
/*	Debugmodus? (=Debug Ausgaben compilieren)	*/
#define	DEBUG					OFF
/*	Debuglogbuch-Datei erstellen? (=Alle Debug Ausgaben in diese Datei)	*/
#define	DEBUG_LOG				NO

/*	Sprache des Programms (fr Text-Strings im Programmcode)	*/
#define	LANGUAGE				GERMAN

/*	Programm nur unter MagiC lauffhig? (MagiC spez. Funktionen Benutzen?)	*/
#define	MAGIC_ONLY				NO

/*	globale VDI Workstation initialisieren? (der ganzen Applikation bekannt)	*/
#define	USE_GLOBAL_VDI			YES
/*	Farbpalette speichern?	*/
#define	SAVE_COLORS				NO

/*	Beim Schliessen eines Fensters (Dialog/Fenster) werden alle Strukturen 
	aus dem Speicher entfert	*/
#define	WIND_CLOSE_IS_REMOVE	YES
/* [GS] 0.35.2a Start */
#define	DIAL_CLOSE_IS_REMOVE	YES
/* Ende; alt:
#define	DIAL_CLOSE_IS_REMOVE	NO
*/

/*	Eigene Globale Tastaturkombinationen? (=bentigt DoUserKeybd())	*/
#define	USE_GLOBAL_KEYBOARD	NO

/*	Eigene Events abfangen?	*/
#define	USE_USER_EVENTS		YES

/*	Menzeile installieren? (Bentigt eine Resource MENU)	*/
#define	USE_MENU					NO

/*	Fenster-Dialoge verwenden? (Dank WDialog)	*/
/* [GS] 0.35.2a Start */
#define	USE_DIALOG				YES
/* Ende; alt:
#define	USE_DIALOG				NO
*/

/*	Normale Fenster verwenden?	*/
#define	USE_WINDOW				YES
/*	setze den Iconify-Name gemss RSC	*/
#define	SET_ICONIFY_NAME		NO
/* ffne fr jedes Fenster eine eigene VDI Workstation	*/
#define	OPEN_VDI_WORKSTATION	NO
/* logisches Raster verwenden	*/
#define	USE_LOGICALRASTER		YES
/*	Toolbar verwenden	*/
#define	USE_TOOLBAR				YES

/*	Fenster-Dateiselektor verwenden?	*/
#define	USE_FILESELECTOR		YES
/*	Fenster-Fontselektor verwenden?	*/
#define	USE_FONTSELECTOR		NO

/*	AV-Protokoll Untersttzung: 
		0	= deaktiviert
		1	= minimal (nur VA_START,AV_SENDCLICK und AV_SENDKEY empfangen)
		2	= normal (AV_PROTOKOL,AV_EXIT,VA_PROTOSTATUS,VA_START,
			  AV_SENDCLICK und AV_SENDKEY)
			  Also anmelden und abmelden beim AV-Server!
		3	= vollstndig (Empfang und Versenden aller mglichen Nachrichten)
			  Bentigt die Prozedur DoVA_Message()!
*/
#define	USE_AV_PROTOCOL		3

/*	Atari Drag&Drop Protokoll Untersttzung	*/
#define	USE_DRAGDROP			YES

/*	Langedateinamen aktivieren (=Pdomain(PD_MINT))	*/
#define	USE_LONGFILENAMES		YES

/*	Lange Editfelder (wie sie von MagiC Untersttzt werden)	*/
#define	USE_LONGEDITFIELDS	NO

/*	BubbleGEM Hilfesystem	*/
#define	USE_BUBBLEGEM			NO
/*	ST-Guide Hilfesystem	*/
#define	USE_STGUIDE				NO

/*	Untersttzung fr das Documen-History Protokoll	*/
#define	USE_DOCUMENTHISTORY	YES

/*	Name der Prototypen-Include-Datei angeben	*/
#define	SPEC_DEFINITION_FILE	"defs.h"

/*	Programmname "schn"	*/
#define	PROGRAM_NAME			"HypView"
/*	Programmname in Grossbuchstaben	*/
#define	PROGRAM_UNAME			"HYP_VIEW"
/*	Dateiname der Resource-Datei	*/
#define	RESOURCE_FILE			"hyp_view.rsc"
/*	Dateiname der Resource-Header-Datei	*/
#define	RESOURCE_HEADER_FILE	"hyp_view.h"
/*	Dateiname der BubbleGEM-Hilfedatei	*/
#define	BUBBLEGEM_FILE			"hyp_view.bgh"
/*	Dateiname der ST-Guide-Hilfedatei	*/
#define	STGUIDE_FILE			"*:\\hyp_view.hyp "

/*	Maximale Zeilenlnge der Konfigurationsdatei	*/
#define	CFG_MAXLINESIZE		80
/*	Soll die Konfigurationsdatei auch im HOME Verzeichnis gesucht werden?	*/
#define	CFG_IN_HOME				YES

/*	Anzahl der mglichen Drag&Drop Formate	*/
#define	MAX_DDFORMAT			8

/*	Anzahl gleichzeitig iconifizierter Fenster	*/
#define	MAX_ICONIFY_PLACE		16
/*	Anzahl der mglichen Rekursions-Ebenen bei Modalen Objekten	*/
#define	MAX_MODALRECURSION	1


/*	Daten fr event_multi	*/
#define EVENTS		MU_MESAG|MU_KEYBD|MU_BUTTON
#define MBCLICKS	2|0x0100
#define MBMASK		3
#define MBSTATE	0
#define MBLOCK1	NULL
#define MBLOCK2	NULL
#define WAIT		0L

/********************************************
 *   end options                            *
 ********************************************/
