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
 * Revision 1.2  2014/07/16 16:31:55  alanh
 * Merge usbtos branch to the trunk.
 *
 * This brings in lots of updates, fixes and new features.
 *
 * - USB stack now available for TOS only (requires the usb ACCessory)
 * - New USB mouse driver
 * - SCSIDRV available for TOS only (possibly for FreeMiNT later)
 * - Unicorn USB adapter driver available for TOS only. Others need porting.
 *
 * Thanks to Roger Burrows for his work on bringing some of the USB code
 * out of assembly language and into C too !
 *
 * Revision 1.1.2.1  2014/06/27 15:39:46  alanh
 * Add all my latest USB for TOS updates.
 * Add USB SCSIDRV support files.
 *
 * Needs cleanup and get ready for merge to the trunk soon.
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
/*#include <atarierr.h>*/

/*****************************************************************************
 * Konstanten
 *****************************************************************************/
#define SCSIRevision 0x0101     /* Version 1.01
                                 * ACHTUNG:
                                 * Die Versionsnummer stellt die Unterrevision im
                                 * Lowbyte dar, die Hauptversion im Highbyte.
                                 * Klienten m�ssen zur grunds�tzlichen Erkennung
                                 * eine verwendbaren Revision das Highbyte pr�fen,
                                 * das Lowbyte lediglich f�r die Verwendung von
                                 * Erweiterungen eine bestimmten Unterrevision.
                                 * Siehe auch scsiio.init_scsiio.
                                 */

#define MAXBUSNO        31      /* maximal m�gliche Busnummer */

#ifdef WIN32
  #define USEASPI
#endif

/* Konvertierung Intel/Motorola */
#ifdef USEASPI

  #define w2mot(A) (((A&0xff)<<8) + ((A&0xff00)>>8))
  #define l2mot(A) (w2mot((LONG)((WORD)A&0xffff)<<16) + (LONG)w2mot((WORD)((A&0xffff0000)>>16)))
#else
  #define w2mot(A) (A)
  #define l2mot(A) (A)
#endif


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
  ULONG BusIds;                       /* abgearbeitete Busnummern
                                       * jeder Treiber mu� bei InuireSCSI das
                                       * mit seiner Busnummer korrespondierende
                                       * Bit setzen.
                                       */
  BYTE  resrvd[28];                   /* f�r Erweiterungen */
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
   /* f�r den Treiber, Treiber m�ssen das definierte Format beachten und nutzen,
    * f�r Applikationen ist das interpretieren dieser Parameter untersagt! */
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
  BOOLEAN CDECL (*TSel)         (WORD     bus,
                                 UWORD    CSB,
                                 UWORD    CSD);
  BOOLEAN CDECL (*TCmd)         (WORD     bus,
                                 BYTE    *Cmd);
  UWORD   CDECL (*TCmdLen)      (WORD     bus,
                                 UWORD    Cmd);
  void    CDECL (*TReset)       (UWORD    bus);
  void    CDECL (*TEOP)         (UWORD    bus);
  void    CDECL (*TPErr)        (UWORD    bus);
  void    CDECL (*TPMism)       (UWORD    bus);
  void    CDECL (*TBLoss)       (UWORD    bus);
  void    CDECL (*TUnknownInt)  (UWORD    bus);
}tTargetHandler;

typedef tTargetHandler *tpTargetHandler;

typedef BYTE tReqData[18];

typedef struct
{
  UWORD Version;                /* Revision in BCD: $0100 = 1.00 */
  
  /* Routinen als Initiator */
  LONG  CDECL (*In)           (tpSCSICmd  Parms);
  LONG  CDECL (*Out)          (tpSCSICmd  Parms);
  
  LONG  CDECL (*InquireSCSI)  (WORD       what,
                               tBusInfo  *Info);
    #define cInqFirst  0
    #define cInqNext   1
  LONG  CDECL (*InquireBus)   (WORD       what,
                               WORD       BusNo,
                               tDevInfo  *Dev);

  LONG  CDECL (*CheckDev)     (WORD       BusNo,
                               const DLONG *SCSIId,
                               char      *Name,
                               UWORD     *Features);
  LONG  CDECL (*RescanBus)    (WORD       BusNo);


  LONG  CDECL (*Open)         (WORD       BusNo,
                               const DLONG *SCSIId,
                               ULONG     *MaxLen);
  LONG  CDECL (*Close)        (tHandle    handle);
  LONG  CDECL (*Error)        (tHandle    handle,
                               WORD       rwflag,
                               WORD       ErrNo);
        #define cErrRead   0
        #define cErrWrite  1
          #define cErrMediach  0
          #define cErrReset    1
  
  /* Routinen als Target (optional) */
  LONG  CDECL (*Install)    (WORD       bus,
                             tpTargetHandler Handler);
  LONG  CDECL (*Deinstall)  (WORD       bus,
                             tpTargetHandler Handler);
  LONG  CDECL (*GetCmd)     (WORD       bus,
                             BYTE      *Cmd);
  LONG  CDECL (*SendData)   (WORD       bus,
                             BYTE      *Buffer,
                             ULONG      Len);
  LONG  CDECL (*GetData)    (WORD       bus,
                             void      *Buffer,
                             ULONG      Len);
  LONG  CDECL (*SendStatus) (WORD       bus,
                             UWORD      Status);
  LONG  CDECL (*SendMsg)    (WORD       bus,
                             UWORD      Msg);
  LONG  CDECL (*GetMsg)     (WORD       bus,
                             UWORD     *Msg);
  
  /* globale Variablen (f�r Targetroutinen) */
  tReqData      *ReqData;
}tScsiCall;
typedef tScsiCall *tpScsiCall;

#endif
