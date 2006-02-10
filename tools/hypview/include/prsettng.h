/*-------------------------------------------------------------------
  		Teil der AES-Bibliothek fr Pure C
 		(aus den verschiedensten Quellen zusammengestellt)
 		von Philipp Donz
 -------------------------------------------------------------------*/

#ifndef	__PRN_SETTINGS__
#define	__PRN_SETTINGS__

/* Einstellbare Farbmodi eines Druckermodus */
#define	CC_MONO			0x0001					/* 2 Grautne */
#define	CC_4_GREY		0x0002					/* 4 Grautne */
#define	CC_8_GREY		0x0004					/* 8 Grautne */
#define	CC_16_GREY		0x0008					/* 16 Grautne */
#define	CC_256_GREY		0x0010					/* 256 Grautne */
#define	CC_32K_GREY		0x0020					/* 32768 Farben in Grautne wandeln */
#define	CC_65K_GREY		0x0040					/* 65536 Farben in Grautne wandeln */
#define	CC_16M_GREY		0x0080					/* 16777216 Farben in Grautne wandeln */

#define	CC_2_COLOR		0x0100					/* 2 Farben */
#define	CC_4_COLOR		0x0200					/* 4 Farben */
#define	CC_8_COLOR		0x0400					/* 8 Farben */
#define	CC_16_COLOR		0x0800					/* 16 Farben */
#define	CC_256_COLOR	0x1000					/* 256 Farben */
#define	CC_32K_COLOR	0x2000					/* 32768 Farben */
#define	CC_65K_COLOR	0x4000					/* 65536 Farben */
#define	CC_16M_COLOR	0x8000					/* 16777216 Farben */

#define	NO_CC_BITS		16


/* Einstellbare Rasterverfahren */
#define	DC_NONE			0							/* keine Rasterverfahren */
#define	DC_FLOYD			1							/* einfacher Floyd-Steinberg */

#define	NO_DC_BITS		1


/* Druckereigenschaften */
#define	PC_FILE			0x0001					/* Drucker kann ber GEMDOS-Dateien angesprochen werden */
#define	PC_SERIAL		0x0002					/* Drucker kann auf der seriellen Schnittstelle angesteuert werden */
#define	PC_PARALLEL		0x0004					/* Drucker kann auf der parallelen Schnittstelle angesteuert werden */
#define	PC_ACSI			0x0008					/* Drucker kann auf der ACSI-Schnittstelle ausgeben */
#define	PC_SCSI			0x0010					/* Drucker kann auf der SCSI-Schnittstelle ausgeben */

#define	PC_BACKGROUND	0x0080					/* Treiber kann im Hintergrund ausdrucken */

#define	PC_SCALING		0x0100					/* Treiber kann Seite skalieren */
#define	PC_COPIES		0x0200					/* Treiber kann Kopien einer Seite erstellen */


/* Moduseigenschaften */
#define	MC_PORTRAIT		0x0001					/* Seite kann im Hochformat ausgegeben werden */
#define	MC_LANDSCAPE	0x0002					/* Seite kann im Querformat ausgegeben werden */
#define	MC_REV_PTRT		0x0004					/* Seite kann um 180 Grad gedreht im Hochformat ausgegeben werden */
#define	MC_REV_LNDSCP	0x0008					/* Seite kann um 180 Grad gedreht im Querformat ausgegeben werden */
#define	MC_ORIENTATION	0x000F

#define	MC_SLCT_CMYK	0x0400					/* Treiber kann bestimmte Farbebenen ausgeben */
#define	MC_CTRST_BRGHT	0x0800					/* Treiber kann Kontrast und Helligkeit verndern */


/* Flags fr plane_flags in PRN_SETTINGS */
#define	PLANE_BLACK		0x0001
#define	PLANE_YELLOW	0x0002
#define	PLANE_MAGENTA	0x0004
#define	PLANE_CYAN		0x0008

/* <driver_mode>*/
#define	DM_BG_PRINTING	0x0001					/* Flag fr Hintergrunddruck */

/* <page_flags> */
#define	PG_EVEN_PAGES	0x0001					/* nur Seiten mit gerader Seitennummer ausgeben */
#define	PG_ODD_PAGES	0x0002					/* nur Seiten mit ungerader Seitennummer ausgeben */

/* <first_page/last_page> */
#define	PG_MIN_PAGE		1
#define	PG_MAX_PAGE		9999

/* <orientation> */
#define	PG_UNKNOWN		0x0000					/* Ausrichtung unbekannt und nicht verstellbar */
#define	PG_PORTRAIT		0x0001					/* Seite im Hochformat ausgeben */
#define	PG_LANDSCAPE	0x0002					/* Seite im Querformat ausgeben */


/* Wird fr die Druckdialoge von WDialog bentigt */
typedef struct _prn_settings
{
	long	magic;				/* 'pset'                                 */
	long	length;				/* (+) Strukturlnge                      */
	long	format;				/* Strukturtyp                            */
	long	reserved;			/* reserviert                             */

	long	page_flags;			/* (+) Flags, u.a. gerade/ungerade Seiten */
									/* 0x0001 = nur Seiten mit gerader Nummer */
									/* 0x0002 = dto. mit ungeraden Nummern    */

	short	first_page;			/* (+) erste zu druckende Seite (min.1)   */
	short	last_page;			/* (+) dto. letzte Seite (max. 9999)      */
	short	no_copies;			/* (+) Anzahl der Kopien                  */

	short	orientation;		/* (+) Drehung                            */
									/* 0x0000 = Ausichtung unbekannt und      */
									/*          nicht verstellbar             */
									/* 0x0001 = Seite im Hochformat ausgeben  */
									/* 0x0002 = Seite im Querformat ausgeben  */

	long	scale;				/* (+) Skalierung: 0x10000L = 100%        */
	short	driver_id;			/* (+) VDI-Gertenummer                   */
	short	driver_type;		/* Typ des eingestellten Treibers         */
	long	driver_mode;		/* Flags, u.a. fr Hintergrunddruck       */
	long	reserved1;			/* reserviert                             */
	long	reserved2;			/* reserviert                             */

	long	printer_id;			/* Druckernummer                          */
	long	mode_id;				/* Modusnummer                            */
	short	mode_hdpi;			/* horizontale Auflsung in dpi           */
	short	mode_vdpi;			/* vertikale Auflsung in dpi             */
	long	quality_id;			/* Druckmodus (hardwremige Qualitt,   */
									/* z.B. Microweave oder Econofast)        */

	long	color_mode;			/* Farbmodus                              */
	long	plane_flags;		/* Flags fr auszugebende Farbebenen      */
									/* (z.B. nur cyan)                        */
	long	dither_mode;		/* Rasterverfahren                        */
	long	dither_value;		/* Parameter fr das Rasterverfahren      */

	long	size_id;				/* Papierformat                           */
	long	type_id;				/* Papiertyp (normal, glossy)             */
	long	input_id;			/* Papiereinzug                           */
	long	output_id;			/* Papierauswurf                          */

	long	contrast;			/* Kontrast:   0x10000L = normal          */
	long	brightness;			/* Helligkeit: 0x1000L  = normal          */
	long	reserved3;			/* reserviert                             */
	long	reserved4;			/* reserviert                             */
	long	reserved5;			/* reserviert                             */
	long	reserved6;			/* reserviert                             */
	long	reserved7;			/* reserviert                             */
	long	reserved8;			/* reserviert                             */
	char	device[128];		/* Dateiname fr den Ausdruck             */

#ifdef __PRINTING__
	TPrint	mac_settings;	/* Einstellung des Mac-Druckertreibers    */
#else
	struct
	{
		unsigned char	inside[120];
	} mac_settings;
#endif
} PRN_SETTINGS;



#endif
