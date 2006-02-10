/*-------------------------------------------------------------------
  		VDI-Bibliothek fr Pure C
 		(aus den verschiedensten Quellen zusammengestellt)
 		von Philipp Donz
 -------------------------------------------------------------------*/
/*------ GDOS-Erweiterungen ----------------------------------------*/
/*------ NVDI-Erweiterungen ----------------------------------------*/
/*------ SPEEDO-Erweiterungen --------------------------------------*/

#if  !defined( __VDI__ )
#define __VDI__

#include <types.h>
#include <prsettng.h>


void cdecl vdi(short *contrl, short *int_in, short *pts_in, short *int_out, short *pts_out);

typedef struct
{
	short contrl[12];
	short intin[132];
	short intout[160];
	short ptsin[160];
	short ptsout[160];
} VDIPARBLK;

extern VDIPARBLK _VDIParBlk;

/*-------------------------------------------------------------------
	Die VDI Stukturen/Typen
 -------------------------------------------------------------------*/

typedef struct
{
	void *fd_addr;
	short fd_w;
	short fd_h;
	short fd_wdwidth;
	short fd_stand;
	short fd_nplanes;
	short fd_r1;
	short fd_r2;
	short fd_r3;
} MFDB;

typedef long fix31;

/* Struktur des Headers eines GEM-Metafiles */
typedef struct
{
	short mf_header;		/* -1, Metafile-Kennung                             */
	short mf_length;		/* Headerlnge in Worten (normalerweise 24)         */
	short mf_version;		/* Versionsnummer des Formats, hier 101 fr 1.01    */
	short mf_ndcrcfl;		/* NDC/RC-Flag, normalerweise 2 (Rasterkoordinaten) */
	short mf_extents[4];	/* optional - maximale Ausmae der Grafik           */
	short mf_pagesz[2];	/* optional - Seitengre in 1/10 mm                */
	short mf_coords[4];	/* optional - Koordinatensystem                     */
	short mf_imgflag;		/* Flag fr durch v_bit_image() eingebundene Bilder */
	short mf_resvd[9];
} METAHDR;


/* Die Namen der Farben */
#define WHITE            0
#define BLACK            1
#define RED              2
#define GREEN            3
#define BLUE             4
#define CYAN             5
#define YELLOW           6
#define MAGENTA          7
#define LWHITE           8
#define LBLACK           9
#define LRED            10
#define LGREEN          11
#define LBLUE           12
#define LCYAN           13
#define LYELLOW         14
#define LMAGENTA        15

#define __COLORS

/* Status des Atari-SLM-Laserdruckers fr v_updwk */
#define NO_ERROR				0
#define PRINTER_NOT_READY	2
#define TONER_EMPTY			3
#define PAPER_EMPTY			5

/* Fehlercodes fr vst_error */
/*#define NO_ERROR				0 */
#define CHAR_NOT_FOUND		1
#define FILE_READERR 		8
#define FILE_OPENERR 		9
#define BAD_FORMAT			10
#define CACHE_FULL			11
#define MISC_ERROR			(-1)

/* Mode fr vst_error */
#define APP_ERROR			0
#define SCREEN_ERROR		1

/* Die Schreibmodi (fr vswr_mode) */
#define MD_REPLACE	1
#define MD_TRANS		2
#define MD_XOR			3
#define MD_ERASE		4

/* Die Linientypen (fr vsl_type) */
#define LT_SOLID			1
#define LT_LONGDASH		2
#define LT_DOTTED			3
#define LT_DASHDOT		4
#define LT_DASHED			5
#define LT_DASHDOTDOT	6
#define LT_USERDEF		7

/* Die Linienenden (fr vsl_ends) */
#define LE_SQUARED	0
#define LE_ARROWED	1
#define LE_ROUNDED	2

/* Die Markertypen (fr vsm_type) */
#define MT_DOT				1
#define MT_PLUS			2
#define MT_ASTERISK		3
#define MT_SQUARE			4
#define MT_DCROSS			5
#define MT_DIAMOND		6

/* Texteffekte (fr vst_effects) */
#define TF_NORMAL			0x00
#define TF_THICKENED		0x01
#define TF_LIGHTENED		0x02
#define TF_SLANTED		0x04
#define TF_UNDERLINED	0x08
#define TF_OUTLINED		0x10
#define TF_SHADOWED		0x20

/* Text-Ausrichtung (fr vst_alignment) */
#define TA_LEFT		0
#define TA_CENTER		1
#define TA_RIGHT		2

#define TA_BASELINE	0
#define TA_HALF		1
#define TA_ASCENT		2
#define TA_BOTTOM		3
#define TA_DESCENT	4
#define TA_TOP			5


/* Mode fr vst_charmap */
#define MAP_BITSTREAM	0
#define MAP_ATARI			1
#define MAP_UNICODE		2		/* for vst_map_mode, NVDI 4 */


/* Modes fr 'track_mode' von vst_kern */
#define TRACK_NONE			0
#define TRACK_NORMAL 		1
#define TRACK_TIGHT			2
#define TRACK_VERYTIGHT		3


/* Modes fr 'pair_mode' von vst_kern */
#define PAIR_OFF				0
#define PAIR_ON				1


/* Modes fr vst_scratch */
#define SCRATCH_BOTH			0
#define SCRATCH_BITMAP		1
#define SCRATCH_NONE			2


/* Flltypen (fr vsf_interior) */
#define FIS_HOLLOW	0
#define FIS_SOLID		1
#define FIS_PATTERN	2
#define FIS_HATCH		3
#define FIS_USER		4


/* vsf_perimeter Modes */
#define PERIMETER_OFF	0
#define PERIMETER_ON		1


#define IP_HOLLOW       0
#define IP_1PATT        1
#define IP_2PATT        2
#define IP_3PATT        3
#define IP_4PATT        4
#define IP_5PATT        5
#define IP_6PATT        6
#define IP_SOLID        7


/* Mode fr v_bez */
#define BEZ_BEZIER		0x01
#define BEZ_POLYLINE		0x00
#define BEZ_NODRAW		0x02


/* Modus fr v_bit_image */
#define IMAGE_LEFT		0
#define IMAGE_CENTER		1
#define IMAGE_RIGHT		2
#define IMAGE_TOP 		0
#define IMAGE_BOTTOM		2


/* Modus fr v_justified */
#define NOJUSTIFY			0
#define JUSTIFY			1


/* vq_color modes */
#define COLOR_REQUESTED		0
#define COLOR_ACTUAL			1


/* Mode fr vqin_mode & vsin_mode */
#define VINMODE_LOCATOR			1
#define VINMODE_VALUATOR			2
#define VINMODE_CHOICE			3
#define VINMODE_STRING			4


/* Modus fr vqt_cachesize */
#define CACHE_CHAR		0
#define CACHE_MISC		1


/* Returnwert von vqt_devinfo */
#define DEV_MISSING			0
#define DEV_INSTALLED		1


/* Returnwert von vqt_name */
#define BITMAP_FONT		0


/* Die Verknpfungen der Raster-Operationen */
#define ALL_WHITE        0
#define S_AND_D          1
#define S_AND_NOTD       2
#define S_ONLY           3
#define NOTS_AND_D       4
#define D_ONLY           5
#define S_XOR_D          6
#define S_OR_D           7
#define NOT_SORD         8
#define NOT_SXORD        9
#define D_INVERT        10
#define NOT_D           10
#define S_OR_NOTD       11
#define NOT_S				12
#define NOTS_OR_D       13
#define NOT_SANDD       14
#define ALL_BLACK       15

#define T_LOGIC_MODE			  0
#define T_COLORIZE			 16		/* Quelle einfrben           */
#define T_DRAW_MODE			 32
#define T_ARITH_MODE			 64		/* Arithmetische Transfermodi */
#define T_DITHER_MODE		128		/* Quelldaten dithern         */

/* Logische Transfermodi fr NVDI-5-Raster-Funktionen */
#define T_LOGIC_COPY				(T_LOGIC_MODE+0)	/* dst = src;                        */
#define T_LOGIC_OR				(T_LOGIC_MODE+1)	/* dst = src OR dst;                 */
#define T_LOGIC_XOR				(T_LOGIC_MODE+2)	/* dst = src XOR dst;                */
#define T_LOGIC_AND				(T_LOGIC_MODE+3)	/* dst = src AND dst;                */
#define T_LOGIC_NOT_COPY		(T_LOGIC_MODE+4)	/* dst = ( NOT src );                */
#define T_LOGIC_NOT_OR			(T_LOGIC_MODE+5)	/* dst = ( NOT src ) OR dst;         */
#define T_LOGIC_NOT_XOR			(T_LOGIC_MODE+6)	/* dst = ( NOT src ) XOR dst;        */
#define T_LOGIC_NOT_AND			(T_LOGIC_MODE+7)	/* dst = ( NOT src ) AND dst;        */

#define T_NOT						T_LOGIC_NOT_COPY

/* Zeichenmodi fr NVDI-5-Raster-Funktionen */
#define T_REPLACE					(T_DRAW_MODE+0)	/* dst = src;                        */
#define T_TRANSPARENT			(T_DRAW_MODE+1)	/* if ( src != bg_col ) dst = src;   */
#define T_HILITE					(T_DRAW_MODE+2)	/* if ( src != bg_col )              */
																/* {                                 */
																/*    if ( dst == bg_col )           */
																/*       dst = hilite_col;           */
																/*    else if ( dst == hilite_col )  */
																/*       dst = bg_col;               */
																/* }                                 */
#define T_REVERS_TRANSPARENT	(T_DRAW_MODE+3)	/* if ( src == bg_col ) dst = src;   */

/* Arithmetische Transfermodi fr NVDI-5-Raster-Funktionen */
#define T_BLEND					(T_ARITH_MODE+0)	/* Quell- und Zielfarbe mischen       */
																/* rgb = RGB( src ) * Gewichtung );   */
																/* rgb += RGB( dst ) * (1-Gewichtung));*/
																/* dst = PIXELWERT( rgb );            */

#define T_ADD						(T_ARITH_MODE+1)	/* Quell- und Zielfarbe addieren      */
																/* rgb = RGB( src ) + RGB( dst )      */
																/* if ( rgb > max_rgb )               */
																/*    rgb = max_rgb;                  */
																/* dst = PIXELWERT( rgb );            */

#define T_ADD_OVER				(T_ARITH_MODE+2)	/* Quell- und Zielfarbe addieren,     */
																/* berlauf nicht abfangen            */
																/* rgb = RGB( src ) + RGB( dst )      */
																/* dst = PIXELWERT( rgb );            */

#define T_SUB						(T_ARITH_MODE+3)	/* Quell- von Zielfarbe subtrahieren  */
																/* rgb = RGB( dst ) - RGB( src )      */
																/* if ( rgb < min_rgb )               */
																/*    rgb = min_rgb;                  */
																/* dst = PIXELWERT( rgb );            */

#define T_MAX						(T_ARITH_MODE+5)	/* maximale RGB-Komponenten           */
																/* rgb = MAX(RGB( dst ), RGB( src ))  */
																/* dst = PIXELWERT( rgb );            */

#define T_SUB_OVER				(T_ARITH_MODE+6)	/* Quell- von Zielfarbe subtrahieren, */
																/* berlauf nicht abfangen            */
																/* rgb = RGB( dst ) - RGB( src )      */
																/* dst = PIXELWERT( rgb );            */

#define T_MIN						(T_ARITH_MODE+7)	/* minimale RGB-Komponenten           */
																/* rgb = MIN(RGB( dst ), RGB( src ))  */
																/* dst = PIXELWERT( rgb );            */


/* Konstanten fr Pixelformate */
#define  PX_1COMP    0x01000000L /* Pixel besteht aus einer benutzten Komponente: Farbindex */
#define  PX_3COMP    0x03000000L /* Pixel besteht aus drei benutzten Komponenten, z.B. RGB */
#define  PX_4COMP    0x04000000L /* Pixel besteht aus vier benutzten Komponenten, z.B. CMYK */

#define  PX_REVERSED 0x00800000L /* Pixel wird in umgekehrter Bytereihenfolge ausgegeben */
#define  PX_xFIRST   0x00400000L /* unbenutzte Bits liegen vor den benutzen (im Motorola-Format betrachtet) */
#define  PX_kFIRST   0x00200000L /* K liegt vor CMY (im Motorola-Format betrachtet) */
#define  PX_aFIRST   0x00100000L /* Alphakanal liegen vor den Farbbits (im Motorola-Format betrachtet) */

#define  PX_PACKED   0x00020000L /* Bits sind aufeinanderfolgend abgelegt */
#define  PX_PLANES   0x00010000L /* Bits sind auf mehrere Ebenen verteilt (Reihenfolge: 0, 1, ..., n) */
#define  PX_IPLANES  0x00000000L /* Bits sind auf mehrere Worte verteilt (Reihenfolge: 0, 1, ..., n) */

#define  PX_USES1    0x00000100L /* 1 Bit des Pixels wird benutzt */
#define  PX_USES2    0x00000200L /* 2 Bit des Pixels werden benutzt */
#define  PX_USES3    0x00000300L /* 3 Bit des Pixels werden benutzt */
#define  PX_USES4    0x00000400L /* 4 Bit des Pixels werden benutzt */
#define  PX_USES8    0x00000800L /* 8 Bit des Pixels werden benutzt */
#define  PX_USES15   0x00000f00L /* 15 Bit des Pixels werden benutzt */
#define  PX_USES16   0x00001000L /* 16 Bit des Pixels werden benutzt */
#define  PX_USES24   0x00001800L /* 24 Bit des Pixels werden benutzt */
#define  PX_USES32   0x00002000L /* 32 Bit des Pixels werden benutzt */
#define  PX_USES48   0x00003000L /* 48 Bit des Pixels werden benutzt */

#define  PX_1BIT     0x00000001L /* Pixel besteht aus 1 Bit */
#define  PX_2BIT     0x00000002L /* Pixel besteht aus 2 Bit */
#define  PX_3BIT     0x00000003L /* Pixel besteht aus 3 Bit */
#define  PX_4BIT     0x00000004L /* Pixel besteht aus 4 Bit */
#define  PX_8BIT     0x00000008L /* Pixel besteht aus 8 Bit */
#define  PX_16BIT    0x00000010L /* Pixel besteht aus 16 Bit */
#define  PX_24BIT    0x00000018L /* Pixel besteht aus 24 Bit */
#define  PX_32BIT    0x00000020L /* Pixel besteht aus 32 Bit */
#define  PX_48BIT    0x00000030L /* Pixel besteht aus 48 Bit */

#define  PX_CMPNTS   0x0f000000L /* Maske f&uuml;r Anzahl der Pixelkomponenten */
#define  PX_FLAGS    0x00f00000L /* Maske f&uuml;r diverse Flags */
#define  PX_PACKING  0x00030000L /* Maske f&uuml;r Pixelformat */
#define  PX_USED     0x00003f00L /* Maske f&uuml;r Anzahl der benutzten Bits */
#define  PX_BITS     0x0000003fL /* Maske f&uuml;r Anzahl der Bits pro Pixel */

/* Pixelformate fr ATARI-Grafik */
#define  PX_ATARI1   ( PX_PACKED + PX_1COMP + PX_USES1 + PX_1BIT )
#define  PX_ATARI2   ( PX_IPLANES + PX_1COMP + PX_USES2 + PX_2BIT )
#define  PX_ATARI4   ( PX_IPLANES + PX_1COMP + PX_USES4 + PX_4BIT )
#define  PX_ATARI8   ( PX_IPLANES + PX_1COMP + PX_USES8 + PX_8BIT )
#define  PX_FALCON15 ( PX_PACKED + PX_3COMP + PX_USES16 + PX_16BIT )

/* Pixelformate fr Mac */
#define  PX_MAC1     ( PX_PACKED + PX_1COMP + PX_USES1 + PX_1BIT )
#define  PX_MAC4     ( PX_PACKED + PX_1COMP + PX_USES4 + PX_4BIT )
#define  PX_MAC8     ( PX_PACKED + PX_1COMP + PX_USES8 + PX_8BIT )
#define  PX_MAC15    ( PX_xFIRST + PX_PACKED + PX_3COMP + PX_USES15 + PX_16BIT )
#define  PX_MAC32    ( PX_xFIRST + PX_PACKED + PX_3COMP + PX_USES24 + PX_32BIT )

/* Pixelformate fr Grafikkarten */
#define  PX_VGA1     ( PX_PACKED + PX_1COMP + PX_USES1 + PX_1BIT )
#define  PX_VGA4     ( PX_PLANES + PX_1COMP + PX_USES4 + PX_4BIT )
#define  PX_VGA8     ( PX_PACKED + PX_1COMP + PX_USES8 + PX_8BIT )
#define  PX_VGA15    ( PX_REVERSED + PX_xFIRST + PX_PACKED + PX_3COMP + PX_USES15 + PX_16BIT )
#define  PX_VGA16    ( PX_REVERSED + PX_PACKED + PX_3COMP + PX_USES16 + PX_16BIT )
#define  PX_VGA24    ( PX_REVERSED + PX_PACKED + PX_3COMP + PX_USES24 + PX_24BIT )
#define  PX_VGA32    ( PX_REVERSED + PX_xFIRST + PX_PACKED + PX_3COMP + PX_USES24 + PX_32BIT )

#define  PX_MATRIX16 ( PX_PACKED + PX_3COMP + PX_USES16 + PX_16BIT )

#define  PX_NOVA32   ( PX_PACKED + PX_3COMP + PX_USES24 + PX_32BIT )

/* Pixelformate fr Drucker */
#define  PX_PRN1     ( PX_PACKED + PX_1COMP + PX_USES1 + PX_1BIT )
#define  PX_PRN8     ( PX_PACKED + PX_1COMP + PX_USES8 + PX_8BIT )
#define  PX_PRN32    ( PX_xFIRST + PX_PACKED + PX_3COMP + PX_USES24 + PX_32BIT )

/* bevorzugte (schnelle) Pixelformate fr Bitmaps */
#define  PX_PREF1    ( PX_PACKED + PX_1COMP + PX_USES1 + PX_1BIT )
#define  PX_PREF2    ( PX_PACKED + PX_1COMP + PX_USES2 + PX_2BIT )
#define  PX_PREF4    ( PX_PACKED + PX_1COMP + PX_USES4 + PX_4BIT )
#define  PX_PREF8    ( PX_PACKED + PX_1COMP + PX_USES8 + PX_8BIT )
#define  PX_PREF15   ( PX_xFIRST + PX_PACKED + PX_3COMP + PX_USES15 + PX_16BIT )
#define  PX_PREF32   ( PX_xFIRST + PX_PACKED + PX_3COMP + PX_USES24 + PX_32BIT )


enum
{
	CSPACE_RGB		=	0x0001,
	CSPACE_ARGB		=	0x0002,
	CSPACE_CMYK		=	0x0004
};


enum
{
	CSPACE_1COMPONENT		=	0x0001,
	CSPACE_2COMPONENTS	=	0x0002,
	CSPACE_3COMPONENTS	= 	0x0003,
	CSPACE_4COMPONENTS	=	0x0004
};


/* Logische Eingabegerte (fr vsin_mode) */
#define DEV_LOCATOR	1
#define DEV_VALUATOR	2
#define DEV_CHOICE	3
#define DEV_STRING	4


/* Eingabe-Modus der Eingabegerte (fr vsin_mode) */
#define MODE_REQUEST	1
#define MODE_SAMPLE	2


/* Die Seiten-Formate (fr v_opnprn) */
#define PAGE_DEFAULT		0
#define PAGE_A3			1		/* DIN A3 */
#define PAGE_A4			2		/* DIN A4 */
#define PAGE_A5			3		/* DIN A5 */
#define PAGE_B5			4		/* DIN B5 */

#define PAGE_LETTER		16		/* letter size      */
#define PAGE_HALF			17		/* half size        */
#define PAGE_LEGAL		18		/* legal size       */
#define PAGE_DOUBLE		19		/* double size      */
#define PAGE_BROAD		20		/* broad sheet size */


/* Flags fr die Font-Informationen ber Fonts (fr vqt_ext_name) */
#define FNT_AEQUIDIST	0x0001
#define FNT_SYMBOL		0x0010


/* Flags fr die Font-Informationen ber das Font-Format (fr vqt_ext_name) */
#define FNT_BITMAP	0x0001
#define FNT_SPEEDO	0x0002
#define FNT_TRUETYPE	0x0004
#define FNT_TYPE1		0x0008


/* Das Mapping von Zeichen (fr vqt_char_index) */
#define CHARIDX_DIRECT	0
#define CHARIDX_ASCII	1
#define CHARIDX_UNICODE	2


/* Mode-Konstanten fr vst_charmap */
#define CHARMODE_DIRECT		 0
#define CHARMODE_ASCII		 1
#define CHARMODE_UNICODE	 2
#define CHARMODE_UNKNOWN	-1


/* Modes fr Track-Kerning (vst_kern) */
#define TRACKMODE_NO					0
#define TRACKMODE_NORMAL			1
#define TRACKMODE_TIGHTY			2
#define TRACKMODE_VERY_TIGHTY		3


/* Modes fr Pair-Kerning (vst_kern) */
#define PAIRKERN_OFF		0
#define PAIRKERN_ON		1


/* Ausrichtung der Seite (v_orient) */
#define ORIENT_INQUIRE		-1
#define ORIENT_NORMAL		0
#define ORIENT_LANDSCAPE	1

/*-------------------------------------------------------------------
	Die Kontrollfunktionen
 -------------------------------------------------------------------*/

void v_opnwk( short *work_in, short *handle, short *work_out );
void v_clswk( const short handle );
void v_opnvwk( short *work_in, short *handle, short *work_out );
void v_clsvwk( const short handle );
void v_clrwk( const short handle );
void v_updwk( const short handle );

short vst_load_fonts( const short handle, const short select );
void vst_unload_fonts( const short handle, const short select );

void vs_clip( const short handle, const short clip_flag, const short *pxyarray );


/*------ NVDI-Erweiterungen ----------------------------------------*/

#include <prsettng.h>

/* Farbbeschreibungen */
typedef struct
{
	short red;
	short green;
	short blue;
} RGB1000;

/* Aufbau der RGB-Farben */
typedef struct
{
	 reserved;	/* reserviert, auf 0 setzen */
	 red;			/* Rot-Anteil: 0..65535     */
	 green;		/* Grn-Anteil: 0..65535    */
	 blue;		/* Blau-Anteil: 0..65535    */
} COLOR_RGB;


/* Aufbau der CMYK-Farben */
typedef struct
{
	 cyan;
	 magenta;
	 yellow;
	 black;
} COLOR_CMYK;


/* Ein Eintrag der Farbtabelle */
typedef union
{
	COLOR_RGB	rgb;
	COLOR_CMYK	cmyk;
} COLOR_ENTRY;


#define	COLOR_TAB_MAGIC	'ctab'

/* Die Farb-Tabelle */
typedef struct
{
	long magic;				/* COLOR_TAB_MAGIC ('ctab') */
	long length;
	long format;				/* Format (0) */
	long reserved;			/* reserviert, auf 0 setzen */

	long map_id;				/* Kennung der Farbtabelle */
	long color_space;		/* Farbraum (z.Zt. nur CSPACE_RGB) */
	long flags;				/* VDI-interne Flags, auf 0 setzen */
	long no_colors;			/* Anzahl der Farbeintrge */

	long reserved1;			/* reserviert, auf 0 setzen */
	long reserved2;			/* reserviert, auf 0 setzen */
	long reserved3;			/* reserviert, auf 0 setzen */
	long reserved4;			/* reserviert, auf 0 setzen */

	COLOR_ENTRY colors[];	/* Die Tabelle der Eintrge */
} COLOR_TAB;


/* vordefinierte Tabelle mit 256 Eintrgen */
typedef struct							/* Farbtabelle */
{
	long magic;				/* COLOR_TAB_MAGIC ('ctab') */
	long length;
	long format;				/* Format (0) */
	long reserved;			/* reserviert, auf 0 setzen */

	long map_id;				/* Kennung der Farbtabelle */
	long color_space;		/* Farbraum (z.Zt. nur CSPACE_RGB) */
	long flags;				/* VDI-interne Flags, auf 0 setzen */
	long no_colors;			/* Anzahl der Farbeintrge */

	long reserved1;			/* reserviert, auf 0 setzen */
	long reserved2;			/* reserviert, auf 0 setzen */
	long reserved3;			/* reserviert, auf 0 setzen */
	long reserved4;			/* reserviert, auf 0 setzen */

	COLOR_ENTRY colors[256];/* Die Tabelle der Eintrge */
} COLOR_TAB256;


/* Zwei Zeiger-Typen auf die Farb-Tabelle */
typedef COLOR_TAB *CTAB_PTR;
typedef COLOR_TAB	*CTAB_REF;


/* Die inverse Farbtabelle - Aufbau nicht dokumentiert und nicht von Bedeutung */
typedef void INVERSE_CTAB;
typedef INVERSE_CTAB *ITAB_PTR;
typedef INVERSE_CTAB *ITAB_REF;


#define CBITMAP_MAGIC	'cbtm'

/* ffentliche Bitmapbeschreibung (Struktur mit Versionsheader) */
typedef struct _gcbitmap
{
   long       magic;      /* Strukturkennung CBITMAP_MAGIC ('cbtm') */
   long       length;     /* Strukturlnge */
   long       format;     /* Strukturformat (0) */
   long       reserved;   /* reserviert (0) */

   unsigned char       *addr;      /* Adresse der Bitmap */
   long       width;      /* Breite einer Zeile in Bytes */
   long       bits;       /* Bittiefe */
   unsigned long      px_format;  /* Pixelformat */

   long       xmin;       /* minimale diskrete x-Koordinate der Bitmap */
   long       ymin;       /* minimale diskrete y-Koordinate der Bitmap */
   long       xmax;       /* maximale diskrete x-Koordinate der Bitmap + 1 */
   long       ymax;       /* maximale diskrete y-Koordinate der Bitmap + 1 */

   CTAB_REF    ctab;       /* Verweis auf die Farbtabelle oder 0L */
   ITAB_REF    itab;       /* Verweis auf die inverse Farbtabelle oder 0L */
   long       reserved0;  /* reserviert (mu auf 0 gesetzt werden) */
   long       reserved1;  /* reserviert (mu auf 0 gesetzt werden) */
} GCBITMAP;


void v_opnprn( short *handle, PRN_SETTINGS *settings, short *work_out );
void v_opnprnwrk( short *handle, short *work_in, PRN_SETTINGS *settings, short *work_out );
void v_opnmatrixprn( short *work_in, short *handle, short *work_out, short max_x, short max_y );
void v_opnmeta( short *work_in, short *handle, short *work_out, const char *filename );
void v_opnbm( short *work_in, const MFDB *bitmap, short *handle, short *work_out );
short v_resize_bm( const short handle, const short width, const short height,
				const long byte_width, const unsigned char *addr );
short v_open_bm( const short base_handle, const GCBITMAP *bitmap, const short zero,
				const short flags, const short pixel_width, const short pixel_height );
void v_clsbm( const short handle );
short v_bez_on( const short handle );
void v_bez_off( const short handle );
short v_flushcache( const short handle );


/*------ SPEEDO-Erweiterungen --------------------------------------*/

void v_set_app_buff( const short handle, void *address, const short nparagraphs );
short v_savecache( const short handle, char *filename );
short v_loadcache( const short handle, const char *filename, const short mode );

/*-------------------------------------------------------------------
	Die Ausgabefunktionen
 -------------------------------------------------------------------*/

void v_pline( const short handle, const short count, short *pxyarray );
void v_pmarker( const short handle, const short count, short *pxyarray );
void v_gtext( const short handle, const short x, const short y, const char *string );
void v_fillarea( const short handle, const short count, short *pxyarray );
void v_cellarray( const short handle, short *pxyarray, const short row_length,
			const short el_used, const short num_rows, const short wrt_mode,
			const short *colarray );
void v_contourfill( const short handle, const short x, const short y, const short index );
void vr_recfl( const short handle, const short *pxyarray );
void v_bar( const short handle, short *pxyarray );
void v_arc( const short handle, const short x, const short y,
		const short radius, const short beg_ang, const short end_ang );
void v_pieslice( const short handle, const short x, const short y,
		const short radius, const short beg_ang, const short end_ang );
void v_circle( const short handle, const short x, const short y, const short radius );
void v_ellipse( const short handle, const short x, const short y,
		const short xradius, const short yradius );
void v_ellarc( const short handle, const short x, const short y,
		const short xradius, const short yradius, const short beg_ang, const short end_ang );
void v_ellpie( const short handle, const short x, const short y,
		const short xradius, const short yradius, const short beg_ang, const short end_ang );
void v_rbox( const short handle, const short *pxyarray );
void v_rfbox( const short handle, const short *pxyarray );
void v_justified( const short handle, const short x, const short y, char *string,
		const short length, const short word_space, const short char_space );


/*------ NVDI-Erweiterungen ----------------------------------------*/

void v_bez( const short handle, const short count, short *pxyarray, char *bezarray,
			short *extent, short *totpts, short *totmoves );
void v_bez_fill( const short handle, const short count, short *pxyarray, unsigned char *bezarray,
			short *extent, short *totpts, short *totmoves );
void v_ftext( const short handle, const short x, const short y, const char *string );
void v_ftext_offset( const short handle, const short x, const short y,
			const char *string, const short *offsets );


/*------ SPEEDO-Erweiterungen --------------------------------------*/


/*-------------------------------------------------------------------
	Die Attributfunktionen
 -------------------------------------------------------------------*/

short vswr_mode( const short handle, const short mode );
void vs_color( const short handle, const short index, const short *rgb_in );
short vsl_type( const short handle, const short style );
void vsl_udsty( const short handle, const short pattern );
short vsl_width( const short handle, const short width );
short vsl_color( const short handle, const short color );
void vsl_ends( const short handle, const short beg_style, const short end_style );
short vsm_type( const short handle, const short symbol );
short vsm_height( const short handle, const short height );
short vsm_color( const short handle, const short color );
void vst_height( const short handle, const short height,
		short *char_width, short *char_height, short *cell_width, short *cell_height );
short vst_point( const short handle, const short point,
		short *char_width, short *char_height, short *cell_width, short *cell_height );
short vst_rotation( const short handle, const short angle );
short vst_font( const short handle, const short fontID );
short vst_color( const short handle, const short color );
short vst_effects( const short handle, const short effect );
void vst_alignment( const short handle, const short hor_in, const short ver_in,
		short *hor_out, short *ver_out );
short vsf_interior( const short handle, const short style );
short vsf_style( const short handle, const short style );
short vsf_color( const short handle, const short color );
short vsf_perimeter( const short handle, const short per_vis );
void vsf_udpat( const short handle, const short *pattern, const short nplanes );


/*------ PC-GEM-Erweiterungen --------------------------------------*/
short vsf_perimeter3( const short handle, const short per_vis, const short style );


/*------ NVDI-Erweiterungen ----------------------------------------*/

short vst_fg_color( const short handle, long color_space, COLOR_ENTRY *fg_color );
short vsf_fg_color( const short handle, long color_space, COLOR_ENTRY *fg_color );
short vsl_fg_color( const short handle, long color_space, COLOR_ENTRY *fg_color );
short vsm_fg_color( const short handle, long color_space, COLOR_ENTRY *fg_color );
short vsr_fg_color( const short handle, long color_space, COLOR_ENTRY *fg_color );
short vst_bg_color( const short handle, long color_space, COLOR_ENTRY *bg_color );
short vsf_bg_color( const short handle, long color_space, COLOR_ENTRY *bg_color );
short vsl_bg_color( const short handle, long color_space, COLOR_ENTRY *bg_color );
short vsm_bg_color( const short handle, long color_space, COLOR_ENTRY *bg_color );
short vsr_bg_color( const short handle, long color_space, COLOR_ENTRY *bg_color );
short vs_ctab( const short handle, COLOR_TAB *ctab );
short vs_ctab_entry( const short handle, const short index,
				const long color_space, COLOR_ENTRY *color );
short vs_dflt_ctab( const short handle );
short vs_hilite_color( const short handle, const long color_space,
				const COLOR_ENTRY *hilite_color );
short vs_min_color( const short handle, const long color_space,
				const COLOR_ENTRY *min_color );
short vs_max_color( const short handle, const long color_space,
				const COLOR_ENTRY *max_color );
short vs_weight_color( const short handle, const long color_space,
				const COLOR_ENTRY *weight_color );
short vst_name( const short handle, const short font_format, const char *font_name,
				char *ret_name );
void vst_width( const short handle, const short width, short *char_width,
					short *char_height, short *cell_width, short *cell_height );
void vst_charmap( const short handle, const short mode );
short vst_map_mode( const short handle, const short mode );
void vst_kern( const short handle, const short track_mode, const short pair_mode,
			short *tracks, short *pairs );
void vst_track_offset( const short handle, const fix31 offset, const short pair_mode,
			short *tracks, short *pairs );
fix31 vst_arbpt32( const short handle, const fix31 height, short *char_width,
			short *char_height, short *cell_width, short *cell_height );
fix31 vst_setsize32( const short handle, const fix31 width, short *char_width,
			short *char_height, short *cell_width, short *cell_height );
short vst_skew( const short handle, const short skew );


/*------ SPEEDO-Erweiterungen --------------------------------------*/

void vst_scratch( const short handle, const short mode );
void vst_error( const short handle, const short mode, short *errorcode );
short vst_arbpt( const short handle, const short point, short *char_width,
			short *char_height, short *cell_width, short *cell_height );
short vst_setsize( const short handle, const short width, short *char_width,
			short *char_height, short *cell_width, short *cell_height );


/*-------------------------------------------------------------------
	Die Rasterfunktionen
 -------------------------------------------------------------------*/

void vro_cpyfm( const short handle, const short wr_mode, const short *pxyarray,
				const MFDB *source, MFDB *dest );
void vrt_cpyfm( const short handle, const short wr_mode, const short *pxyarray,
				const MFDB *source, MFDB *dest, const short color[2] );
void vr_trnfm( const short handle, const MFDB *source, MFDB *dest );
void v_get_pixel( const short handle, const short x, const short y,
		short *pix_value, short *col_index );


/*------ NVDI-Erweiterungen ----------------------------------------*/

void vr_transfer_bits( const short handle, const GCBITMAP *src_bm, const GCBITMAP *dst_bm,
			const short *src_rect, const short *dst_rect, const short mode );


/*-------------------------------------------------------------------
	Die Eingabefunktionen
 -------------------------------------------------------------------*/

short vsin_mode( const short handle, const short dev_type, const short mode );
void vrq_locator( const short handle, const short x_in, const short y_in,
		short *x_out, short *y_out, short *term );
short vsm_locator( const short handle, const short x_in, const short y_in,
		short *x_out, short *y_out, short *term );
void vrq_valuator( const short handle, const short value_in, short *value_out, short *term );
void vsm_valuator( const short handle, const short value_in,
		short *value_out, short *term, short *status );
void vrq_choice( const short handle, const short choice_in, short *choice_out );
short vsm_choice( const short handle, short *choice );
void vrq_string( const short handle, const short max_length, const short echo_mode,
		const short *echo_xy, char *string );
short vsm_string( const short handle, const short max_length, const short echo_mode,
		const short *echo_xy, char *string );
void vsc_form( const short handle, const short *pcur_form );
void vex_timv( const short handle, const short (*tim_addr)(),
		short (**otim_addr)(), short *tim_conv );
void v_show_c( const short handle, const short reset );
void v_hide_c( const short handle );
void vq_mouse( const short handle, short *pstatus, short *x, short *y );
void vex_butv( const short handle, const short (*pusrcode)(), short (**psavcode)() );
void vex_motv( const short handle, const short (*pusrcode)(), short (**psavcode)() );
void vex_curv( const short handle, const short (*pusrcode)(), short (**psavcode)() );
void vq_key_s( const short handle, short *pstatus );


/*------ MilanTOS-Erweiterungen ------------------------------------*/

void vex_wheelv( const short handle, const short (*pusrcode)(void), short (**psavcode)(void) );


/*-------------------------------------------------------------------
	Die Auskunftsfunktionen
 -------------------------------------------------------------------*/

/* Rckgabewerte von vq_vgdos */
#define GDOS_NONE      -2L            /* no GDOS installed           */
#define GDOS_FSM       0x5F46534DL    /* '_FSM' - FSMGDOS installed  */
#define GDOS_FNT       0x5F464E54L    /* '_FNT' - FONTGDOS installed */

short vq_gdos( void );
long vq_vgdos( void );

void vq_extnd( const short handle, const short owflag, short *workout );
short vq_color( const short handle, const short color, const short set_flag, short *rgb );
void vql_attributes( const short handle, short *attrib );
void vqm_attributes( const short handle, short *attrib );
void vqf_attributes( const short handle, short *attrib );
void vqt_attributes( const short handle, short *attrib );
void vqt_extent( const short handle, const char *string, short *extent );
short vqt_width( const short handle, const short character, short *cell_width,
			short *left_delta, short *right_delta );
short vqt_name( const short handle, const short element_num, char *name );
void vq_cellarray( const short handle, const short *pxyarray, const short row_length,
				const short num_rows, short *el_used, short *rows_used, short *status,
				short *colarray );
void vqin_mode( const short handle, const short dev_type, short *input_mode );
void vqt_fontinfo( const short handle, short *minADE, short *maxADE, short *distances,
			short *maxwidth, short *effects );


/*------ PC-GEM-Erweiterungen --------------------------------------*/

void vqt_justified( const short handle, const short x, const short y, const char *string,
			const short length, const short word_space, const short char_space, short *offsets );


/*------ NVDI-Erweiterungen ----------------------------------------*/

/* Font-Info-Struktur (fr vqt_xfntinfo) */
typedef struct
{
	long size;					/* Lnge der Struktur, mu vor vqt_xfntinfo() gesetzt werden */
	short format;				/* Fontformat, z.B. 4 f&uuml;r TrueType                      */
	short id;					/* Font-ID, z.B. 6059                                        */
	short index;				/* Index                                                     */
	char	font_name[50];		/* vollstndiger Fontname, z.B. "Century 725 Italic BT"      */
	char	family_name[50];	/* Name der Fontfamilie, z.B. "Century725 BT"                */
	char	style_name[50];	/* Name des Fontstils, z.B. "Italic"                         */
	char	file_name1[200];	/* Name der 1. Fontdatei, z.B. "C:\FONTS\TT1059M_.TTF"       */
	char	file_name2[200];	/* Name der optionalen 2. Fontdatei                          */
	char	file_name3[200];	/* Name der optionalen 3. Fontdatei                          */
	short pt_cnt;				/* Anzahl der Punkthhen fr vst_point(), z.B. 10            */
	short pt_sizes[64];		/* verfgbare Punkthhen, z.B.                               */
									/* {8, 9, 10, 11, 12, 14, 18, 24, 36, 48}                    */
} XFNT_INFO;

void vq_scrninfo( const short handle, short *work_out );
short vqt_ext_name( const short handle, const short element_num, char *name,
				 *font_format,  *flags );
 vqt_char_index( const short handle, const  src_index,
					const short src_mode, const short dst_mode );
long vqt_fg_color( const short handle, COLOR_ENTRY *fg_color );
long vqf_fg_color( const short handle, COLOR_ENTRY *fg_color );
long vql_fg_color( const short handle, COLOR_ENTRY *fg_color );
long vqm_fg_color( const short handle, COLOR_ENTRY *fg_color );
long vqr_fg_color( const short handle, COLOR_ENTRY *fg_color );
long vqt_bg_color( const short handle, COLOR_ENTRY *bg_color );
long vqf_bg_color( const short handle, COLOR_ENTRY *bg_color );
long vql_bg_color( const short handle, COLOR_ENTRY *bg_color );
long vqm_bg_color( const short handle, COLOR_ENTRY *bg_color );
long vqr_bg_color( const short handle, COLOR_ENTRY *bg_color );
unsigned long v_color2value( const short handle, const long color_space, COLOR_ENTRY *color );
long v_value2color( const short handle, const unsigned long value, COLOR_ENTRY *color );
long v_color2nearest( const short handle, const long color_space, const COLOR_ENTRY *color,
			COLOR_ENTRY *nearest );
long vq_px_format( const short handle, unsigned long *px_format );
short vq_ctab( const short handle, const long ctab_length, COLOR_TAB *ctab );
long vq_ctab_entry( const short handle, const short index, COLOR_ENTRY *color );
long vq_ctab_id( const short handle );
short v_ctab_idx2vdi( const short handle, const short index );
short v_ctab_vdi2idx( const short handle, const short index );
long v_ctab_idx2value( const short handle, const short index );
long v_get_ctab_id( const short handle );
short vq_dflt_ctab( const short handle, long ctab_length, COLOR_TAB *ctab );
COLOR_TAB *v_create_ctab( const short handle, long color_space, unsigned long px_format );
short v_delete_ctab( const short handle, COLOR_TAB *ctab);
ITAB_REF v_create_itab( const short handle, COLOR_TAB *ctab, short bits );
short v_delete_itab( const short handle, ITAB_REF *itab );
long vq_hilite_color( const short handle, COLOR_ENTRY *hilite_color );
long vq_min_color( const short handle, COLOR_ENTRY *min_color );
long vq_max_color( const short handle, COLOR_ENTRY *max_color );
long vq_weight_color( const short handle, COLOR_ENTRY *weight_color );
short vqt_xfntinfo( const short handle, const short flags, const short id,
				const short index, XFNT_INFO *info );
short vqt_name_and_id( const short handle, const short font_format,
				const char *font_name, char *ret_name );
void vqt_fontheader( const short handle, void *buffer, char *tdf_name );
void vqt_trackkern( const short handle, fix31 *x_offset, fix31 *y_offset );
void vqt_pairkern( const short handle, const short index1, const short index2,
				fix31 *x_offset, fix31 *y_offset );
void v_getbitmap_info( const short handle, const short index,
			fix31 *x_advance, fix31 *y_advance, fix31 *x_offset, fix31 *y_offset,
			short *width, short *height, short **bitmap );
void vqt_f_extent( const short handle, const char *string, short *extent );
void vqt_real_extent( const short handle, const short x, const short y,
			const char *string, short *extent );
void v_getoutline( const short handle, const short index, short *xyarray,
			unsigned char *bezarray, const short max_pts, short *count );
short v_get_outline( const short handle, const short index, const short x_offset,
			const short y_offset, short *xyarray, unsigned char *bezarray, const short max_pts );
void vqt_advance32( const short handle, const short index, fix31 *x_advance, fix31 *y_advance );
void vq_devinfo( const short handle, const short device, boolean *dev_open,
			char *file_name, char *device_name );
boolean vq_ext_devinfo( const short handle, const short device, boolean *dev_exists,
			char *file_path, char *file_name, char *name );


/*------ SPEEDO-Erweiterungen --------------------------------------*/

void vqt_advance( const short handle, const short ch, short *x_advance, short *y_advance,
				short *x_remainder, short *y_remainder );
void vqt_devinfo( const short handle, const short devnum,
				boolean *devexists, char *devstr );
void vqt_get_table( const short handle, short **map );
void vqt_cachesize( const short handle, const short which_cache, long *size );

/*-------------------------------------------------------------------
	Die Escapes
 -------------------------------------------------------------------*/

void vq_chcells( const short handle, short *rows, short *cols );
void v_exit_cur( const short handle );
void v_enter_cur( const short handle );
void v_curup( const short handle );
void v_curdown( const short handle );
void v_curright( const short handle );
void v_curleft( const short handle );
void v_curhome( const short handle );
void v_eeos( const short handle );
void v_eeol( const short handle );
void v_curaddress( const short handle, const short row, const short col );
void v_curtext( const short handle, const char *string );
void v_rvon( const short handle );
void v_rvoff( const short handle );
void vq_curaddress( const short handle, short *row, short *col );
short vq_tabstatus( const short handle );
void v_hardcopy( const short handle );
void v_dspcur( const short handle, const short x, const short y );
void v_rmcur( const short handle );
void v_form_adv( const short handle );
void v_output_window( const short handle, const short *pxyarray );
void v_clear_disp_list( const short handle );
void v_bit_image( const short handle, const char *filename, const short aspect,
				const short x_scale, const short y_scale, const short h_align,
				const short v_align, const short *pxyarray );
void vq_scan( const short handle, short *g_slice, short *g_page,
			short *a_slice, short *a_page, short *div_fac );
void v_alpha_text( const short handle, const char *string );
void vt_resolution( const short handle, const short xres, const short yres,
			short *xset, short *yset );
void vt_axis( const short handle, const short xres, const short yres,
			short *xset, short *yset );
void vt_origin( const short handle, const short xorigin, const short yorigin );
void vq_tdimensions( const short handle, short *xdim, short *ydim );
void vt_alignment( const short handle, const short dx, const short dy );
void vsp_film( const short handle, const short index, const short lightness );
short vqp_filmname( const short handle, const short index, char *name );
void vsc_expose( const short handle, const short status );
void v_meta_extents( const short handle, const short min_x, const short min_y,
			const short max_x, const short max_y );
void v_write_meta( const short handle, const short num_intin, const short *a_intin,
			const short num_ptsin, const short *a_ptsin );
void vm_pagesize( const short handle, const short pgwidth, const short pgheight );
void vm_coords( const short handle, const short llx, const short lly,
			const short urx, const short ury );
void vm_filename( const short handle, const char *filename );
void v_offset( const short handle, const short offset );
void v_fontinit( const short handle, const void *font_header );
void v_escape2000( const short handle, const short times );


/*------ PC-GEM-Erweiterungen --------------------------------------*/

short vs_palette( const short handle, const short palette );
void v_sound( const short handle, const short frequency, const short duration );
short vs_mute( const short handle, const short action );


/*------ NVDI-Erweiterungen ----------------------------------------*/

short v_orient( const short handle, const short orient );
short v_copies( short handle, short count );
short v_trays( const short handle, const short input, const short output,
				short *set_input, short *set_output );
short vq_tray_names( const short handle, char *input_name, char *output_name,
				short *input, short *output );
short v_page_size( const short handle, const short page_id );
short vq_page_name( const short handle, const short page_id, char *page_name,
				long *page_width, long *page_height );
fixed vq_prn_scaling( const short handle );
short vs_calibrate( const short handle, const boolean flag, const RGB1000 *table );
short vq_calibrate( const short handle, boolean *flag );
void v_bez_qual( const short handle, const short qual, short *set_qual );
short vs_document_info( const short handle, const short type, const char *s, short wchar );




/*-------------------------------------------------------------------
	Diverse Utility Routinen
 -------------------------------------------------------------------*/
short fix31ToPixel( fix31 a );


#endif
