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
	Tabulatorweite: 3
	Kommentare ab: Spalte 60											*Spalte 60*
*/
#include <string.h>
#include <errno.h>
#ifdef __GNUC__
	#include <fcntl.h>
	#include <mintbind.h>
	#include <signal.h>
#else
	#include <tos.h>
#endif
#include <gem.h>
#include "dragdrop.h"

/*----------------------------------------------------------------------------------------*/ 
/* Drag & Drop - Pipe ffnen (fuer den Sender)															*/
/* Funktionsresultat:	Handle der Pipe, -1 fuer Fehler oder -2 fuer Fehler bei appl_write	*/
/*	app_id:					ID des Senders (der eigenen Applikation)									*/
/*	rcvr_id:					ID des Empfaengers																	*/
/*	window:					Handle des Empaefnger-Fensters													*/
/*	mx:						x-Koordinate der Maus beim Loslassen oder -1								*/
/*	my:						y-Koordinate der Maus beim Loslassen oder -1								*/
/*	kbstate:					Status der Kontrolltasten														*/
/*	format:					Feld fuer die max. 8 vom Empfaenger unterstuetzten Formate				*/
/*	oldpipesig:				Zeiger auf den alten Signal-Dispatcher										*/
/*----------------------------------------------------------------------------------------*/ 
short
ddcreate( short	app_id, short rcvr_id, short window, short mx, short my, short kbstate, unsigned long format[8], void **oldpipesig )
{
	char	pipe[24];
	short	mbuf[8];
	long	handle_mask;
	short	handle, i;

	strcpy( pipe, "u:\\pipe\\dragdrop.aa" );
	pipe[18] = 'a' - 1;
	do
	{
		pipe[18]++;					/* letzten Buchstaben weitersetzen */
		if ( pipe[18] > 'z' )				/* kein Buchstabe des Alphabets? */
		{
			pipe[17]++;				/* ersten Buchstaben der Extension aendern */
			if ( pipe[17] > 'z' )			/* liess sich keine Pipe oeffnen? */
				return( -1 );
		}

		handle = (short) Fcreate( pipe, 0x02 );		/* Pipe anlegen, 0x02 bedeutet, da EOF zurueckgeliefert wird, */
								/* wenn die Pipe von niemanden zum Lesen geoeffnet wurde */
	} while ( handle == EACCES );

	if ( handle < 0 )					/* liess sich die Pipe nicht anlegen? */
		return( handle );

	mbuf[0] = AP_DRAGDROP;					/* Drap&Drop-Message senden */
	mbuf[1] = app_id;					/* ID der eigenen Applikation */
	mbuf[2] = 0;
	mbuf[3] = window;					/* Handle des Fensters */
	mbuf[4] = mx;						/* x-Koordinate der Maus */
	mbuf[5] = my;						/* y-Koordinate der Maus */
	mbuf[6] = kbstate;					/* Tastatur-Status */
	mbuf[7] = (((short) pipe[17]) << 8 ) + pipe[18];	/* Endung des Pipe-Namens */

	if ( appl_write( rcvr_id, 16, mbuf ) == 0 )		/* Fehler bei appl_write()? */
	{
		Fclose( handle );				/* Pipe schliessen */
		return( -2 );
	}

	handle_mask = 1L << handle;
	i = (short)Fselect( DD_TIMEOUT, &handle_mask, 0L, 0L );	/* auf Antwort warten */

	if ( i && handle_mask )					/* kein Timeout? */
	{
		char	reply;
		
		if ( Fread( handle, 1L, &reply ) == 1 )		/* Antwort vom Empfaenger lesen */
		{
			if ( reply == DD_OK )			/* alles in Ordnung? */
			{
				if ( Fread( handle, DD_EXTSIZE, format ) == DD_EXTSIZE )	/* unterstuetzte Formate lesen */
				{
					*oldpipesig = (void *)Psignal( SIGPIPE, (void *) SIG_IGN );	/* Dispatcher ausklinken */
					return( handle );
				}
			}
		}
	}

	Fclose( handle );					/* Pipe schliessen */
	return( -1 );
}


/*----------------------------------------------------------------------------------------*/ 
/* Drag & Drop - berprfen ob der Empfaenger ein Format akzeptiert								*/
/* Funktionsresultat:	DD_OK: Empfnger unterstuetzt das Format										*/
/*								DD_EXT: Empfaenger akzeptiert das Format nicht							*/
/*								DD_LEN: Daten sind zu lang fuer den Empfaenger								*/
/*								DD_NAK: Fehler bei Kommunikation												*/								
/*	handle:					Handle der Pipe																	*/
/*	format:					Kruezel fuer das Format															*/
/*	name:						Beschreibung des Formats als C-String										*/
/*	size:						Laenge der zu sendenen Daten													*/
/*----------------------------------------------------------------------------------------*/ 
short
ddstry( short handle, unsigned long format, char *name, long size )
{
	long	str_len;
	short	hdr_len;
	
	str_len = strlen( name ) + 1;					/* Laenge des Strings inklusive Nullbyte */
	hdr_len = 4 + 4 + (short) str_len;				/* Laenge des Headers */

	if ( Fwrite( handle, 2, &hdr_len ) == 2 )			/* Laenge des Headers senden */
	{
		long	written;
		
		written = Fwrite( handle, 4, &format );			/* Formatkruezel */
		written += Fwrite( handle, 4, &size );			/* Laenge der zu sendenden Daten */
		written += Fwrite( handle, str_len, name );		/* Beschreibung des Formats als C-String */

		if ( written == hdr_len )				/* liess sich der Header schreiben? */
		{
			char	reply;
			
			if ( Fread( handle, 1, &reply ) == 1 )
				return( reply );											/* Antwort zurueckliefern */
		}
	}	
	return( DD_NAK );
}

/*----------------------------------------------------------------------------------------*/ 
/* Drag & Drop - Pipe schliessen																				*/
/*	handle:					Handle der Pipe																	*/
/* oldpipesig:				Zeiger auf den alten Signalhandler											*/
/*----------------------------------------------------------------------------------------*/ 
void
ddclose( short handle, void *oldpipesig )
{
	Psignal( SIGPIPE, oldpipesig );					/* wieder alten Dispatcher eintragen */
	Fclose( handle );						/* Pipe schliessen */
}

/*----------------------------------------------------------------------------------------*/ 
/* Drag & Drop - Pipe oeffnen (fuer den Empfnger)														*/
/* Funktionsresultat:	Handle der Pipe oder -1 (Fehler)												*/
/* pipe:						Zeiger auf den Namen der Pipe ("DRAGDROP.??")							*/
/* format:					Zeiger auf Array mit unterstuetzten Datenformaten						*/
/* oldpipesig:				Zeiger auf den Zeiger auf den alten Signalhandler						*/
/*----------------------------------------------------------------------------------------*/ 
short
ddopen( char *pipe, unsigned long format[8], void **oldpipesig )
{
	short	handle;
	char	reply;

	handle = (short) Fopen( pipe, O_RDWR );					/* Handle der Pipe erfragen	*/
	if ( handle < 0 )
		return( -1 );

	reply = DD_OK;								/* Programm unterstuetzt Drag & Drop	*/

	*oldpipesig = (void *)Psignal( SIGPIPE, (void *) SIG_IGN );		/* Signal ignorieren	*/

	if ( Fwrite( handle, 1, &reply ) == 1 )
	{
		if ( Fwrite( handle, DD_EXTSIZE, format ) == DD_EXTSIZE )
			return( handle );
	}

	ddclose( handle, *oldpipesig );						/* Pipe schliessen */
	return( -1 );
}

/*----------------------------------------------------------------------------------------*/ 
/* Header fuer Drag & Drop einlesen																			*/
/* Funktionsresultat:	0 Fehler 1: alles in Ordnung													*/
/*	handle:					Handle der Pipe																	*/
/* name:						Zeiger auf Array fuer den Datennamen											*/
/* format:					Zeiger auf ein Long, das das Datenformat anzeigt						*/
/* size:						Zeiger auf ein Long fuer die Laenge der Daten								*/
/*----------------------------------------------------------------------------------------*/ 
short
ddrtry( short handle, char *name, unsigned long *format, long *size )
{
	short	hdr_len;

	if ( Fread( handle, 2, &hdr_len ) == 2 )				/* Headerlaenge auslesen	*/
	{
		if ( hdr_len >= 9 )						/* kompletter Header?	*/
		{
			if ( Fread( handle, 4, format ) == 4 )			/* Datentyp auslesen	*/
			{
				if ( Fread( handle, 4, size ) == 4 )		/* Laenge der Daten in Bytes auslesen */
				{	
					short	name_len;
					
					name_len = hdr_len -= 8;		/* Laenge des Namens inklusive Nullbyte */

					if ( name_len > DD_NAMEMAX )				
						name_len = DD_NAMEMAX;

					if ( Fread( handle, name_len, name ) == name_len )	/* Datennamen auslesen	*/
					{
						char	buf[64];
					
						hdr_len -= name_len;
	
						while ( hdr_len > 64 )		/* Headerrest auslesen	*/
						{
							Fread( handle, 64, buf );
							hdr_len -= 64;
						}
		
						if ( hdr_len > 0 )
							Fread( handle, hdr_len, buf );
	
						return( 1 );
					}
				}
			}
		}
	}
	return( 0 );															/* Fehler */
}

/*----------------------------------------------------------------------------------------*/ 
/* Meldung an den Drag & Drop - Initiator senden														*/
/* Funktionsresultat:	0: Fehler 1: alles in Ordnung													*/
/*	handle:					Handle der Pipe																	*/
/* msg:						Nachrichtennummer																	*/
/*----------------------------------------------------------------------------------------*/ 
short
ddreply( short handle, short msg )
{
	if ( Fwrite( handle, 1, ((char *) &msg ) + 1 ) != 1 )		/* Fehler? */
	{
		Fclose( handle );					/* Pipe schliessen */
		return( 0 );
	}
	return( 1 );
}
