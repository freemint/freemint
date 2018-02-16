/*
 * HypView - (c) 2001 - 2006 Philipp Donze
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

#ifndef _dl_user_h
#define _dl_user_h
/********************************************
 *   begin options                          *
 ********************************************/
/* #define GENERATE_DIAGS */
/* Debug mode? (=add debugging output while compiling) */
#define	DEBUG					OFF
/* Create debug log file? (=put all debugging output into file) */
#define	DEBUG_LOG				NO

/* Main language for this application (=built in error messages) */
#define	LANGUAGE				ENGLISH

/* Use MagiC specific functions? (=limit usage on MagiC compatible systems) */
#define	MAGIC_ONLY				NO

/* Use a global VDI workstation? */
#define	USE_GLOBAL_VDI			YES
/*	Save color palette on init?	*/
#define	SAVE_COLORS				NO

/* Closing a window/dialog will automatically remove all related structures */
#define	WIND_CLOSE_IS_REMOVE	YES
#define	DIAL_CLOSE_IS_REMOVE	YES

/* Use a global keyboard shortcut handler (=needs DoUserKeybd() in dl_user.c) */
#define	USE_GLOBAL_KEYBOARD	NO

/* Use user specific events? (=Pass events to DoUserEvents() before handling) */
#define	USE_USER_EVENTS		YES

/* Install/remove menubar on initialization? (Needs a resource object MENU) */
#define	USE_MENU		NO

/* Use window dialog management routines? (Based on WDialog) */
#define	USE_DIALOG		YES

/* Use normal window management routines? */
#define	USE_WINDOW		YES
/* Automatically set the title when iconifying according to RSC file */
#define	SET_ICONIFY_NAME	NO
/* Open a separate VDI workstation for each new window */
#define	OPEN_VDI_WORKSTATION	NO
/* Use a logical raster for computations (=not the pixel raster) */
#define	USE_LOGICALRASTER	YES
/* Use toolbar routines */
#define	USE_TOOLBAR		YES

/* Use fileselector routines */
#define	USE_FILESELECTOR	YES
/* Use fontselector routines */
#define	USE_FONTSELECTOR	NO

/* Level of AV protocol support:
	0 = none
	1 = minimal (limit on VA_START, AV_SENDCLICK and AV_SENDKEY reception)
	2 = normal (AV_PROTOKOL, AV_EXIT, VA_PROTOSTATUS, VA_START,
	    AV_SENDCLICK and AV_SENDKEY)
	3 = full (Sending and reception of all possible messages)
	    Needs DoVA_Message()!
*/
#define	USE_AV_PROTOCOL     3

/* Support Drag&Drop protokol */
#define	USE_DRAGDROP        YES

/* Support long file names (=Pdomain(PD_MINT)) */
#define	USE_LONGFILENAMES   YES

/* Enable support for long edit fields (As in MagiC)	*/
#define	USE_LONGEDITFIELDS  NO

/* BubbleGEM help system */
#define	USE_BUBBLEGEM       NO
/* ST-Guide help system */
#define	USE_STGUIDE         NO

/* Document history protocol */
#define	USE_DOCUMENTHISTORY	YES

/* Name of user specific prototype */
#define	SPEC_DEFINITION_FILE  "defs.h"

/* "nice" form of the application name */
#define	PROGRAM_NAME		"HypView"
/* Application name in uppercase letters */
#define	PROGRAM_UNAME		"HYP_VIEW"
/* Name of the resource file */
#define	RESOURCE_FILE		"hyp_view.rsc"
/* Name of the resource header file */
#define	RESOURCE_HEADER_FILE	"hyp_view.h"
/* Name of the BubbleGEM header file */
#define	BUBBLEGEM_FILE		"hyp_view.bgh"
/* Name of the ST-Guide help file */
#define	STGUIDE_FILE		"*:\\hyp_view.hyp "

/* Maximum line lenght in the configuration file */
#define	CFG_MAXLINESIZE		80
/* Look for configuration file in the HOME folder? */
#define	CFG_IN_HOME		YES

/* Number of supported Drag&Drop formats */
#define	MAX_DDFORMAT		8

/* Maximum number of simultaneous iconified windows */
#define	MAX_ICONIFY_PLACE	16
/* Maximum number of recursion levels for modal dialogs */
#define	MAX_MODALRECURSION	1


/* event_multi parameters	*/
#define EVENTS		MU_MESAG|MU_KEYBD|MU_BUTTON
#define MBCLICKS	2|0x0100
#define MBMASK		3
#define MBSTATE		0
#define MBLOCK1		NULL
#define MBLOCK2		NULL
#define WAIT		0L

/********************************************
 *   end options                            *
 ********************************************/
#endif     /* _dl_user_h */
