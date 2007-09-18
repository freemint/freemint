/*
 * Daynaport SCSI/Link driver for FreeMiNT.
 * GNU C conversion by Miro Kropacek, <miro.kropacek@gmail.com>
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * Copyright (c) 2007 Roger Burrows.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/****************************************************************************
 *
 * Typen f�r SCSI-Calls in C
 *
 * $Source$
 *
 * $Revision$
 *
 * $Author$
 *
 * $Date$
 *
 * $State$
 *
 *****************************************************************************
 * History:
 *
 * $Log$
 * Revision 1.1  2007/09/18 14:59:21  fna
 * Added scsidrv xif module to make teh scsidrv interface as network
 * interface available (contributed by Miro Kropacek).
 *
 *
 * Revision 1.8a 1999/02/12  rfb
 * .changed parameter BusNo in function definitions from type WORD to UWORD,
 *	for consistency with definition of tBusInfo
 * .added #defines for BOOLEAN, cdecl
 * .removed #include for atarierr.h
 *
 * Revision 1.8  1996/02/14  11:33:52  Steffen_Engel
 * keine globalen Kommandostrukturen mehr
 *
 * Revision 1.7  1996/01/25  17:53:16  Steffen_Engel
 * Tippfehler bei PARITYERROR korrigiert
 *
 * Revision 1.6  1995/11/28  19:14:14  S_Engel
 * *** empty log message ***
 *
 * Revision 1.5  1995/11/14  22:15:58  S_Engel
 * Kleine Korrekturen
 * aktualisiert auf aktuellen Stand
 *
 * Revision 1.4  1995/10/22  15:43:34  S_Engel
 * Kommentare leicht �berarbeitet
 *
 * Revision 1.3  1995/10/13  22:30:54  S_Engel
 * GetMsg in Struktur eingef�gt
 *
 * Revision 1.2  1995/10/11  10:21:34  S_Engel
 * Handle als LONG, Disconnect auf Bit4 verlegt
 *
 * Revision 1.1  1995/10/03  12:49:42  S_Engel
 * Initial revision
 *
 *
 ****************************************************************************/


#ifndef __SCSIDEFS_H
#define __SCSIDEFS_H

#include "portab.h"
#define BOOLEAN	WORD

/*****************************************************************************
 * Konstanten
 *****************************************************************************/
#define SCSIRevision 0x0100                     /* Version 1.00 */

#define MAXBUSNO        31       /* maximal m�gliche Busnummer */


/* SCSI-Fehlermeldungen f�r In und Out */

#define NOSCSIERROR      0L /* Kein Fehler                                   */
#define SELECTERROR     -1L /* Fehler beim Selektieren                       */
#define STATUSERROR     -2L /* Default-Fehler                                */
#define PHASEERROR      -3L /* ung�ltige Phase                               */
#define BSYERROR        -4L /* BSY verloren                                  */
#define BUSERROR        -5L /* Busfehler bei DMA-�bertragung                 */
#define TRANSERROR      -6L /* Fehler beim DMA-Transfer (nichts �bertragen)  */
#define FREEERROR       -7L /* Bus wird nicht mehr freigegeben               */
#define TIMEOUTERROR    -8L /* Timeout                                       */
#define DATATOOLONG     -9L /* Daten f�r ACSI-Softtransfer zu lang           */
#define LINKERROR      -10L /* Fehler beim Senden des Linked-Command (ACSI)  */
#define TIMEOUTARBIT   -11L /* Timeout bei der Arbitrierung                  */
#define PENDINGERROR   -12L /* auf diesem handle ist ein Fehler vermerkt     */
#define PARITYERROR    -13L /* Transfer verursachte Parity-Fehler            */


/*****************************************************************************
 * Typen
 *****************************************************************************/

/*
  in portab.h zu definieren:

  BYTE      : 8 Bit signed (char)
  UBYTE     : 8 Bit unsigned (unsigned char)
  UWORD     : 16 Bit ganzzahlig positiv (unsigned word)
  ULONG     : 32 Bit ganzzahlig positiv (unsigned long)
  WORD      : 16 Bit ganzzahlig (word)
  LONG      : 32 Bit ganzzahlig (long)
  BOOLEAN   : WORD
              TRUE  : 1
              FALSE : 0
  DLONG     : 64 Bit unsigned
*/

typedef struct
{
  ULONG hi;
  ULONG lo;
} DLONG;

typedef struct{
  ULONG BusIds;
  BYTE  resrvd[28];
}tPrivate;

typedef WORD *tHandle;                /* Zeiger auf BusFeatures */

typedef struct
{
  tHandle Handle;                     /* Handle f�r Bus und Ger�t             */
  BYTE  *Cmd;                         /* Zeiger auf CmdBlock                  */
  UWORD CmdLen;                       /* L�nge des Cmd-Block (f�r ACSI n�tig) */
  void  *Buffer;                      /* Datenpuffer                          */
  ULONG TransferLen;                  /* �bertragungsl�nge                    */
  BYTE  *SenseBuffer;                 /* Puffer f�r ReqSense (18 Bytes)       */
  ULONG Timeout;                      /* Timeout in 1/200 sec                 */
  UWORD Flags;                        /* Bitvektor f�r Ablaufw�nsche          */
    #define Disconnect 0x10           /* versuche disconnect                  */

}tSCSICmd;
typedef tSCSICmd *tpSCSICmd;


typedef struct
{
  tPrivate Private;
   /* f�r den Treiber */
  char  BusName[20];
   /* zB 'SCSI', 'ACSI', 'PAK-SCSI' */
  UWORD BusNo;
   /* Nummer, unter der der Bus anzusprechen ist */
  UWORD Features;
      #define cArbit     0x01    /* auf dem Bus wird arbitriert                          */
      #define cAllCmds   0x02    /* hier k�nnen ale SCSI-Cmds abgesetzt werden           */
      #define cTargCtrl  0x04    /* Das Target steuert den Ablauf (so soll's sein!)      */
      #define cTarget    0x08    /* auf diesem Bus kann man sich als Target installieren */
      #define cCanDisconnect 0x10 /* Disconnect ist m�glich                             */
      #define cScatterGather 0x20 /* scatter gather bei virtuellem RAM m�glich */
  /* bis zu 16 Features, die der Bus kann, zB Arbit,
   * Full-SCSI (alle SCSI-Cmds im Gegensatz zu ACSI)
   * Target oder Initiator gesteuert
   * Ein SCSI-Handle ist auch ein Zeiger auf eine Kopie dieser Information!
   */
  ULONG MaxLen;
  /* maximale Transferl�nge auf diesem Bus (in Bytes)
   * entspricht zB bei ACSI der Gr��e des FRB
   */
}tBusInfo;

typedef struct
{
  BYTE Private[32];
  DLONG SCSIId;
}tDevInfo;


typedef struct ttargethandler
{
  struct  ttargethandler *next;
  BOOLEAN _cdecl (*TSel)         (WORD     bus,
                                 UWORD    CSB,
                                 UWORD    CSD);
  BOOLEAN _cdecl (*TCmd)         (WORD     bus,
                                 BYTE    *Cmd);
  UWORD   _cdecl (*TCmdLen)      (WORD     bus,
                                 UWORD    Cmd);
  void    _cdecl (*TReset)       (UWORD    bus);
  void    _cdecl (*TEOP)         (UWORD    bus);
  void    _cdecl (*TPErr)        (UWORD    bus);
  void    _cdecl (*TPMism)       (UWORD    bus);
  void    _cdecl (*TBLoss)       (UWORD    bus);
  void    _cdecl (*TUnknownInt)  (UWORD    bus);
}tTargetHandler;

typedef tTargetHandler *tpTargetHandler;

typedef BYTE tReqData[18];

typedef struct
{
  UWORD Version;                /* Revision in BCD: $0100 = 1.00 */
  
  /* Routinen als Initiator */
  LONG  _cdecl (*In)           (tpSCSICmd  Parms);
  LONG  _cdecl (*Out)          (tpSCSICmd  Parms);
  
  LONG  _cdecl (*InquireSCSI)  (WORD       what,
                               tBusInfo  *Info);
    #define cInqFirst  0
    #define cInqNext   1
  LONG  _cdecl (*InquireBus)   (WORD       what,
                               UWORD      BusNo,
                               tDevInfo  *Dev);

  LONG  _cdecl (*CheckDev)     (UWORD      BusNo,
                               const DLONG *SCSIId,
                               char      *Name,
                               UWORD     *Features);
  LONG  _cdecl (*RescanBus)    (UWORD      BusNo);


  LONG  _cdecl (*Open)         (UWORD      BusNo,
                               const DLONG *SCSIId,
                               ULONG     *MaxLen);
  LONG  _cdecl (*Close)        (tHandle    handle);
  LONG  _cdecl (*Error)        (tHandle    handle,
                               WORD       rwflag,
                               WORD       ErrNo);
        #define cErrRead   0
        #define cErrWrite  1
          #define cErrMediach  0
          #define cErrReset    1
  
  /* Routinen als Target (optional) */
  LONG  _cdecl (*Install)    (WORD       bus,
                             tpTargetHandler Handler);
  LONG  _cdecl (*Deinstall)  (WORD       bus,
                             tpTargetHandler Handler);
  LONG  _cdecl (*GetCmd)     (WORD       bus,
                             BYTE      *Cmd);
  LONG  _cdecl (*SendData)   (WORD       bus,
                             BYTE      *Buffer,
                             ULONG      Len);
  LONG  _cdecl (*GetData)    (WORD       bus,
                             void      *Buffer,
                             ULONG      Len);
  LONG  _cdecl (*SendStatus) (WORD       bus,
                             UWORD      Status);
  LONG  _cdecl (*SendMsg)    (WORD       bus,
                             UWORD      Msg);
  LONG  _cdecl (*GetMsg)     (WORD       bus,
                             UWORD     *Msg);
  
  /* globale Variablen (f�r Targetroutinen) */
  tReqData      *ReqData;
}tScsiCall;
typedef tScsiCall *tpScsiCall;

#endif
