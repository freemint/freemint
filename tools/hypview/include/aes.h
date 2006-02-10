/*-------------------------------------------------------------------
  		AES-Bibliothek fr Pure C
 		(aus den verschiedensten Quellen zusammengestellt)
 		von Philipp Donz
 -------------------------------------------------------------------*/

#if  !defined( __AES__ )
#define __AES__

#include <types.h>
#include <prsettng.h>


void cdecl aes(short *contrl, short *global, short *int_in, short *int_out, long *addr_in, long *addr_out);


/* Das global-Array als Struktur */
typedef struct
{
	short ap_version;
	short ap_count;
	short ap_id;
	long ap_private;
	long ap_ptree;
	long ap_pmem;
	short ap_lmem;
	short ap_nplanes;
	long ap_res;
	short ap_bvdisk;
	short ap_bvhard;
} GlobalArray;


/*-------------------------------------------------------------------
	Die globalen Variablen
 -------------------------------------------------------------------*/

/* Zeiger auf GLOBAL-Struktur des Hauptthreads */
extern GlobalArray _globl;

/* Flag, ob Anwendung als Application oder Accessory luft */
extern short _app;


/*-------------------------------------------------------------------
	Die AES Stukturen/Typen
 -------------------------------------------------------------------*/

/* Beschreibung eines Rechteckes */
typedef struct
{
	short g_x;
	short g_y;
	short g_w;
	short g_h;
} GRECT;

/* Ein paar "forward"-Deklarationen der Strukturen */
struct object;
struct parm_block;


/* Die TEDINFO-Struktur */
typedef struct text_edinfo
{
	char	*te_ptext;         /* Zeiger auf einen String          */
	char	*te_ptmplt;        /* Zeiger auf die Stringmaske       */
	char	*te_pvalid;        /* Zeiger auf den Gltigkeitsstring */
	short	te_font;           /* Zeichensatz                      */
	short	te_fontid;
	short	te_just;           /* Justierung des Texts             */
	short	te_color;          /* Farbe                            */
	short	te_fontsize;
	short	te_thickness;      /* Rahmenbreite                     */
	short	te_txtlen;         /* Maximale Lnge des Textes        */
	short	te_tmplen;         /* Lnge der Stringmaske            */
} TEDINFO;

/* Zugriff fr die PureC-Komponenten te_junk1 und te_junk2 */
#define te_junk1	te_fontid
#define te_junk2	te_fontsize


/* Die ICONBLK-Struktur */
typedef struct icon_block
{
	short	*ib_pmask;			/* Zeiger auf die Maske          */
	short	*ib_pdata;			/* Zeiger auf das Icon-Bild      */
	char	*ib_ptext;			/* Zeiger auf einen String       */
	short	ib_char;				/* In den unteren 8 Bit das dar- */
									/* zustellende Zeichen, in den   */
									/* oberen 8 Bit die Farbe des    */
									/* gesetzten (obere 4 Bit) und   */
									/* des gelschten (untere 4 Bit) */
									/* Bits des Bitmuster IB_PDATA   */
	short	ib_xchar;			/* x-Koordinate des Buchstabens  */
	short	ib_ychar;			/* y-Koordinate des Buchstabens  */
	short	ib_xicon;			/* x-Koordinate des Icons        */
	short	ib_yicon;			/* y-Koordinate des Icons        */
	short	ib_wicon;			/* Breite des Icons              */
	short	ib_hicon;			/* Hhe des Icons                */
	short	ib_xtext;			/* x-Koordinate des Textes       */
	short	ib_ytext;			/* y-Koordinate des Textes       */
	short	ib_wtext;			/* Breite des Textes             */
	short	ib_htext;			/* Hhe des Textes               */
} ICONBLK;


/* Farb-Icon-Struktur */
typedef struct cicon_data
{
	short num_planes;
	short *col_data;
	short *col_mask;
	short *sel_data;
	short *sel_mask;
	struct cicon_data  *next_res;
} CICON;


/* Die IconBlk-Struktur fr Farb- & Monochrom-Icons */
typedef struct cicon_blk
{
	ICONBLK	monoblk;
	CICON		*mainlist;
} CICONBLK;


/* Die BITBLK-Struktur */
typedef struct bit_block
{
	short	*bi_pdata;		/* Zeiger auf die Grafikdaten    */
	short	bi_wb;			/* Breite des Bildes in Bytes    */
	short	bi_hl;			/* Hhe in Linien                */
	short	bi_x;				/* x-Position                    */
	short	bi_y;				/* y-Position                    */
	short	bi_color;		/* Vordergrundfarbe              */
} BITBLK;


/* Die USERBLK-Struktur (ACS verwendet stattdessen die AUSERBLK-Struktur!) */
#ifndef __STDC__	/* Struktur nicht unter ANSI-C */
typedef struct user_block
{
	short cdecl (*ub_code)(struct parm_block *pb);	/* Zeiger auf die Zeichen-Funktion */
	long	ub_parm;												/* Optionaler Parameter            */
} USERBLK;
#endif


/* Die Komponente ob_spec in OBJECT als Bitfeld */
typedef struct
{
	unsigned character	:	8;
	signed   framesize	:	8;
	unsigned framecol		:	4;
	unsigned textcol		:	4;
	unsigned textmode		:	1;
	unsigned fillpattern	:	3;
	unsigned interiorcol	:	4;
} bfobspec;

/* Alias-Name der Struktur */
#define BFOBSPEC	bfobspec


/* Die verschiedenen Mglichkeiten der Komponente ob_spec in OBJECT */
typedef union obspecptr
{
	long		index;				/* fr das RSC-C-output      */
	union obspecptr *indirect;	/* Zeiger auf ob_spec        */
	bfobspec	obspec;				/* Bitfield                  */
	TEDINFO	*tedinfo;			/* Zeiger auf TEDINFO        */
	ICONBLK	*iconblk;			/* Zeiger auf ICONBLK        */
	CICONBLK *ciconblk;
	BITBLK	*bitblk;				/* Zeiger auf BITBLK         */
#ifndef __STDC__
	USERBLK	*userblk;			/* Zeiger auf USERBLK        */
#endif
	char		*free_string;		/* zeiger auf String         */
} OBSPEC;


/* Die OBJECT-Struktur fr die AES */
typedef struct object
{
	short ob_next;
	short ob_head;
	short ob_tail;
	 ob_type;
	 ob_flags;
	 ob_state;
	OBSPEC ob_spec;
	short ob_x;
	short ob_y;
	short ob_width;
	short ob_height;
} OBJECT;


/* Die PARMBLK-Struktur (fr UserDefs USERDEF & AUSERDEF wichtig!) */
typedef struct parm_block
{
	OBJECT	*pb_tree;		/* Zeiger auf den Objektbaum       */
	short		pb_obj;			/* Nummer des Objekts              */
	short		pb_prevstate;	/* Vorheriger Objektstatus         */
	short		pb_currstate;	/* Neuer Objektstatus              */
	short		pb_x;				/* Objektkoordinaten               */
	short		pb_y;				/*      -"-                        */
	short		pb_w;				/*      -"-                        */
	short		pb_h;				/*      -"-                        */
	short		pb_xc;			/* Clipping-Koordinaten            */
	short		pb_yc;			/*      -"-                        */
	short		pb_wc;			/*      -"-                        */
	short		pb_hc;			/*      -"-                        */
	long		pb_parm;			/* Optionale Parameter aus USERBLK */
} PARMBLK;


/* MultiTOS Drag&Drop definitions */
#define	DD_OK			0
#define	DD_NAK		1
#define	DD_EXT		2
#define	DD_LEN		3

#define	DD_TIMEOUT	4000		/* Timeout in ms */

#define	DD_NUMEXTS	8			/* Anzahl der Formate */
#define	DD_EXTSIZE	32			/* Lnge des Formatfelds */

#define	DD_FNAME		"U:\\PIPE\\DRAGDROP.AA"
#define	DD_NAMEMAX	128		/* maximale Lnge eines Formatnamens */
#define	DD_HDRMIN	9			/* minimale Lnge des Drag&Drop-Headers */
#define	DD_HDRMAX	( 8 + DD_NAMEMAX )	/* maximale Lnge */


/* Scrollable textedit objects */
typedef struct _xted
{
	char	*xte_ptmplt;
	char	*xte_pvalid;
	short	xte_vislen;
	short	xte_scroll;
} XTED;



/*-------------------------------------------------------------------
	Die APPL-Funktionen
 -------------------------------------------------------------------*/

/* Struktur der Ereignisse (fr appl_tplay und appl_trecord) */
typedef struct
{
	long type;
	long what;
} APPLRECORD;

/* Die Funktions-Codes fr appl_getinfo */
#define AES_LARGEFONT	0
#define AES_SMALLFONT	1
#define AES_SYSTEM		2
#define AES_LANGUAGE		3
#define AES_PROCESS		4
#define AES_PCGEM			5
#define AES_INQUIRE		6
#define AES_MOUSE			8
#define AES_MENU			9
#define AES_SHELL			10
#define AES_WINDOW		11
#define AES_MESSAGE		12
#define AES_OBJECT		13
#define AES_FORM			14


/* Die Font-Typ fr AES_LARGEFONT und AES_SMALLFONT, appl_getinfo */
#define SYSTEM_FONT			0
#define OUTLINE_FONT 		1


/* Die Sprachen fr AES_LANGUAGE, appl_getinfo */
#define AESLANG_ENGLISH		0
#define AESLANG_GERMAN		1
#define AESLANG_FRENCH		2
#define AESLANG_SPANISH		4
#define AESLANG_ITALIAN		5
#define AESLANG_SWEDISH		6


/* Funktionscodes fr appl_search */
#define APP_FIRST		0
#define APP_NEXT		1


/* Proze-Typen (fr appl_search) */
#define APP_SYSTEM			0x01
#define APP_APPLICATION		0x02
#define APP_ACCESSORY		0x04
#define APP_SHELL				0x08


/* Event-Typen (fr appl_tplay und appl_trecord) */
#define APPEVNT_TIMER		0
#define APPEVNT_BUTTON		1
#define APPEVNT_MOUSE		2
#define APPEVNT_KEYBOARD	3


/* Keine Daten vorhanden (appl_read) - MagiC & MultiTOS */
#define APR_NOWAIT			-1


/* Konstanten fr ap_cwhat der Funktion appl_control */
#define APC_HIDE		10
#define APC_SHOW		11
#define APC_TOP		12
#define APC_HIDENOT	13
#define APC_INFO		14
#define APC_MENU		15
#define APC_WIDGETS	16


/* Flags fr den Integer, auf den ap_cout bei APC_INFO zeigt (appl_control) */
#define APCI_HIDDEN	0x01
#define APCI_HASMBAR	0x02
#define APCI_HASDESK	0x04


/* Gre des Puffers in short fr APC_WIDGETS (appl_control) */
#define MINWINOBJ		12


/* Tastatur-Status (fr appl_trecord, evnt_button, evnt_mouse, evnt_multi) */
#define K_RSHIFT	0x0001
#define K_LSHIFT	0x0002
#define K_CTRL		0x0004
#define K_ALT		0x0008


short mt_appl_init( GlobalArray *globl );
short mt_appl_read( const short rwid, const short length, void *pbuff, GlobalArray *globl );
short mt_appl_write( const short rwid, const short length, void *pbuff, GlobalArray *globl );
short mt_appl_find( const char *pname, GlobalArray *globl );
short mt_appl_tplay( APPLRECORD *tbuffer, const short tlength, const short tscale,
				GlobalArray *globl );
short mt_appl_trecord( APPLRECORD *tbuffer, const short tlength, GlobalArray *globl );
short mt_appl_bvset( const  bvdisk, const  bvhard, GlobalArray *globl );
short mt_appl_yield( GlobalArray *globl );
short mt_appl_search( const short mode, const char *fname, short *type, short *ap_id, GlobalArray *globl );
short mt_appl_exit( GlobalArray *globl );
short mt_appl_getinfo( const short type, short *out1, short *out2, short *out3, short *out4,
				GlobalArray *globl );
short mt_appl_control( const short ap_cid, const short ap_cwhat, void *ap_cout,
									GlobalArray *globl );


#define appl_init()										mt_appl_init(&_globl)
#define appl_read(rwid, length, pbuff)				mt_appl_read(rwid, length, pbuff, &_globl)
#define appl_write(rwid, length, pbuff)			mt_appl_write(rwid, length, pbuff, &_globl)
#define appl_find(pname)								mt_appl_find(pname, &_globl)
#define appl_search(mode, fname, type, ap_id)	mt_appl_search(mode, fname, type, ap_id, &_globl)
#define appl_tplay(buffer, tlength, tscale)		mt_appl_tplay(buffer, tlength, tscale, &_globl)
#define appl_trecord(tbuffer, tlength)				mt_appl_trecord(tbuffer, tlength, &_globl)
#define appl_bvset(a, b)								mt_appl_bvset(a, b, &_globl)
#define appl_yield()										mt_appl_yield(&_globl)
#define appl_exit()										mt_appl_exit(&_globl)
#define appl_getinfo(type, out1, out2, out3, out4)	\
																mt_appl_getinfo(type, out1, out2, out3, out4, &_globl)
#define appl_control(ap_cid, ap_cwhat, ap_cout)	mt_appl_control(ap_cid, ap_cwhat, ap_cout, &_globl)


/*-------------------------------------------------------------------
	Die EVNT-Funktionen
 -------------------------------------------------------------------*/

/*	Maus-Position/Status und Tastatur-Status (evnt_button, evnt_multi)	*/
typedef struct
{
	short	x;
	short	y;
	short	bstate;
	short	kstate;
} EVNTDATA;

/* Mausrechteck fr EVNT_multi() */
typedef struct
{
	short m_out;
	short m_x;
	short m_y;
	short m_w;
	short m_h;
} MOBLK;

typedef struct
{
	short mwhich;         /* Art der Ereignisse */
	short mx;             /* x-Koordinate des Mauszeigers */
	short my;             /* y-Koordinate des Mauszeigers */
	short mbutton;        /* gedrckte Maustaste */
	short kstate;         /* Status der Sondertasten (kbshift) */
	short key;            /* Scancode der gedrckten Taste */
	short mclicks;        /* Anzahl der Mausklicks */
	short reserved[9];    /* reserviert */
	short msg[16];        /* Message-Buffer */
} EVNT;


/* Flags fr die Maus-berwachung (evnt_mouse, evnt_multi, evnt_event) */
#define MO_ENTER	0
#define MO_LEAVE	1

/* Ereignis-Flags (fr evnt_multi) */
#define MU_KEYBD				0x0001
#define MU_BUTTON				0x0002
#define MU_M1					0x0004
#define MU_M2					0x0008
#define MU_MESAG				0x0010
#define MU_TIMER				0x0020
#define MU_WHEEL				0x0040		/* XaAES */
#define MU_MX					0x0080		/* XaAES */
#define MU_NORM_KEYBD		0x0100		/* XaAES */
#define MU_DYNAMIC_KEYBD	0x0200		/* XaAES */


#define MN_SELECTED	10
#define WM_REDRAW		20
#define WM_TOPPED		21
#define WM_CLOSED		22
#define WM_FULLED		23
#define WM_ARROWED	24
#define WM_HSLID	 	25
#define WM_VSLID	 	26
#define WM_SIZED	 	27
#define WM_MOVED	 	28
#define WM_NEWTOP		29
#define WM_UNTOPPED 	30							/* GEM  2.x	*/
#define WM_ONTOP		31							/* AES 4.0	*/
#define WM_BOTTOMED 	33							/* AES 4.1	*/
#define WM_ICONIFY	34							/* AES 4.1	*/
#define WM_UNICONIFY	35							/* AES 4.1	*/
#define WM_ALLICONIFY	36						/* AES 4.1	*/
#define AC_OPEN		40
#define AC_CLOSE		41
#define CT_UPDATE		50
#define CT_MOVE		51
#define CT_NEWTOP		52
#define CT_KEY			53
#define AP_TERM		50							/* AES 4.0	*/
#define AP_TFAIL		51							/* AES 4.0	*/
#define AP_RESCHG		57							/* AES 4.0	*/
#define SHUT_COMPLETED	60						/* AES 4.0	*/
#define RESCH_COMPLETED	61						/* AES 4.0	*/
#define AP_DRAGDROP 	63							/* AES 4.0	*/
#define SH_WDRAW		72							/* MultiTOS	*/
#define SC_CHANGED	80							/* */
#define PRN_CHANGED	82
#define FNT_CHANGED	83
#define THR_EXIT		88							/* MagiC 4.5	*/
#define PA_EXIT		89							/* MagiC 3	*/
#define CH_EXIT		90							/* MultiTOS	*/
#define WM_M_BDROPPED	100					/* KAOS 1.4	*/
#define SM_M_SPECIAL	101						/* MAG!X		*/
#define SM_M_RES2		102						/* MAG!X		*/
#define SM_M_RES3		103						/* MAG!X		*/
#define SM_M_RES4		104						/* MAG!X		*/
#define SM_M_RES5		105						/* MAG!X		*/
#define SM_M_RES6		106						/* MAG!X		*/
#define SM_M_RES7		107						/* MAG!X		*/
#define SM_M_RES8		108						/* MAG!X		*/
#define SM_M_RES9		109						/* MAG!X		*/
#define WM_SHADED		22360						/* [WM_SHADED apid 0 win 0 0 0 0] */
#define WM_UNSHADED	22361						/* [WM_UNSHADED apid 0 win 0 0 0 0] */

/* SM_M_SPECIAL codes */
#define SMC_TIDY_UP		0						/* MagiC 2	*/
#define SMC_TERMINATE	1						/* MagiC 2	*/
#define SMC_SWITCH		2						/* MagiC 2	*/
#define SMC_FREEZE		3						/* MagiC 2	*/
#define SMC_UNFREEZE		4						/* MagiC 2	*/
#define SMC_RES5			5						/* MagiC 2	*/
#define SMC_UNHIDEALL	6						/* MagiC 3.1	*/
#define SMC_HIDEOTHERS	7						/* MagiC 3.1	*/
#define SMC_HIDEACT		8						/* MagiC 3.1	*/


/* Button-Masken fr evnt_button und evnt_multi */
#define LEFT_BUTTON		0x0001
#define RIGHT_BUTTON 	0x0002
#define MIDDLE_BUTTON	0x0004


/* Flag fr evnt_dclick */
#define EDC_INQUIRE		0
#define EDC_SET			1


short mt_evnt_keybd( GlobalArray *globl );
short mt_evnt_button( const short ev_bclicks, const  ev_bmask,
				const  ev_bstate, EVNTDATA *ev, GlobalArray *globl );
short mt_evnt_xbutton( const short ev_bclicks, const  ev_bmask,
				const  ev_bstate, EVNTDATA *ev, short *ev_bwhlpbuff,
				GlobalArray *globl );
short mt_evnt_mouse( const short ev_moflags, const GRECT *ev_rect, EVNTDATA *ev,
				GlobalArray *globl );
short mt_evnt_mesag( short *pbuff, GlobalArray *globl );
short mt_evnt_timer( const unsigned long count, GlobalArray *globl );
short mt_evnt_multi( const short ev_mflags, const short ev_mbclicks, const short ev_mbmask,
				const short ev_mbstate, const short ev_mm1flags, const GRECT *ev_mm1,
				const short ev_mm2flags, const GRECT *ev_mm2,
				short *ev_mmgpbuff, const long ev_mtimer,
				EVNTDATA *ev_mm, short *ev_mkreturn, short *ev_mbreturn, GlobalArray *globl );
short mt_evnt_xmulti( const short ev_mflags, const short ev_mbclicks, const short ev_mbmask,
				const short ev_mbstate, const short ev_mm1flags, const GRECT *ev_mm1,
				const short ev_mm2flags, const GRECT *ev_mm2,
				short *ev_mmgpbuff, const long ev_mtimer,
				EVNTDATA *ev_mm, short *ev_mkreturn, short *ev_mbreturn, short *ev_mwhlpbuff,
				GlobalArray *globl );
void	mt_EVNT_multi( short ev_mflags, short ev_mbclicks, short ev_mbmask, 
							short ev_mbstate,
							MOBLK *m1, MOBLK *m2, unsigned long ev_mtimer,
							EVNT *event, GlobalArray *global );
short mt_evnt_dclick( const short rate, const short setit, GlobalArray *globl );


#define evnt_keybd()							mt_evnt_keybd(&_globl)
#define evnt_button(a, b, c, d) 			mt_evnt_button(a, b, c, d, &_globl)
#define evnt_xbutton(a, b, c, d, e) 	mt_evnt_xbutton(a, b, c, d, e, &_globl)
#define evnt_mouse(a, b, c)				mt_evnt_mouse(a, b, c, &_globl)
#define evnt_mesag(pbuff)					mt_evnt_mesag(pbuff, &_globl)
#define evnt_timer(timer) 					mt_evnt_timer(timer, &_globl)
#define evnt_multi(a, b, c, d, e, f, g, h, i, j, k, l, m) \
													mt_evnt_multi(a, b, c, d, e, f, g, h, i, j, k, l, m, &_globl)
#define evnt_xmulti(a, b, c, d, e, f, g, h, i, j, k, l, m, n) \
													mt_evnt_xmulti(a, b, c, d, e, f, g, h, i, j, k, l, m, n, &_globl)
#define EVNT_multi(a, b, c, d, e, f, g, h) \
													mt_EVNT_multi(a, b, c, d, e, f, g, h, &_globl)
#define evnt_dclick(rate, setit)			mt_evnt_dclick(rate, setit, &_globl)


/*-------------------------------------------------------------------
	Die MENU-Funktionen
 -------------------------------------------------------------------*/

/* Struktur fr menu_attach */
typedef struct
{
	OBJECT *mn_tree;
	short	mn_menu;
	short	mn_item;
	short	mn_scroll;
	short	mn_keystate;
} MENU;


/* Men-Einstellungen (fr menu_settings) */
typedef struct
{
	long display;
	long drag;
	long delay;
	long speed;
	short height;
} MN_SET;


/* Funktions-Codes fr menu_attach */
#define ME_INQUIRE		0
#define ME_ATTACH			1
#define ME_REMOVE			2


/* Scroll-Infos (fr Struktur MENU, menu_attach) */
#define SCROLL_NO		0
#define SCROLL_YES	1


/* Funktions-Codes fr menu_bar */
#define MENU_INQUIRE		-1
#define MENU_REMOVE		0
#define MENU_INSTALL		1
#define MENU_GETMODE		3			/* Menleistenmodus abfragen             */
#define MENU_SETMODE		4			/* Menleistenmodus setzen               */
#define MENU_UPDATE		5			/* Update des Systemteils der Menleiste */
#define MENU_INSTL      100		/* MAG!X */


/* Bit-Struktur fr MENU_SETMODE */
#define  MENU_HIDDEN		0x0001	/* Menleiste nur bei Bedarf sichtbar */
#define  MENU_PULLDOWN	0x0002	/* Pulldown-Mens                     */
#define  MENU_SHADOWED	0x0004	/* Menleistenboxen mit Schatten      */


/* Modus fr menu_icheck */
#define UNCHECK			0
#define CHECK				1


/* Modus fr menu_ienable */
#define DISABLE			0
#define ENABLE 			1


/* Modus fr menu_istart */
#define MIS_GETALIGN 	0
#define MIS_SETALIGN 	1


/* Modus fr menu_popup  */
#define SCROLL_LISTBOX	-1


/* Spezielle Ap-ID fr's Registrieren des Prozenamens */
#define REG_NEWNAME		-1


/* Modus fr menu_tnormal */
#define HIGHLIGHT			0
#define UNHIGHLIGHT		1


short mt_menu_bar( OBJECT *tree, const short show, GlobalArray *globl );
short mt_menu_icheck( OBJECT *tree, const short item, const short check, GlobalArray *globl );
short mt_menu_ienable( OBJECT *tree, const short item, const short enable,
				GlobalArray *globl );
short mt_menu_tnormal( OBJECT *tree, const short title, const short normal,
				GlobalArray *globl );
short mt_menu_text( OBJECT *tree, const short item, char *text, GlobalArray *globl );
short mt_menu_register( const short apid, char *string, GlobalArray *globl );
short mt_menu_unregister( const short apid, GlobalArray *globl );
short mt_menu_click( const short click, const short setit, GlobalArray *globl );
short mt_menu_attach( const short flag, OBJECT *tree, const short item, MENU *mdata,
				GlobalArray *globl );
short mt_menu_istart( const short flag, OBJECT *tree, const short imenu, const short item,
			GlobalArray *globl );
short mt_menu_popup( MENU *menu, const short xpos, const short ypos, MENU *mdata,
			GlobalArray *globl );
short mt_menu_settings( const short flag, MN_SET *set, GlobalArray *globl );


#define menu_bar(tree, show)						mt_menu_bar(tree, show, &_globl)
#define menu_icheck(tree, item, check)			mt_menu_icheck(tree, item, check, &_globl)
#define menu_ienable(tree, item, enable)		mt_menu_ienable(tree, item, enable, &_globl)
#define menu_tnormal(tree, title, normal)		mt_menu_tnormal(tree, title, normal, &_globl)
#define menu_text(tree, item, text)				mt_menu_text(tree, item, text, &_globl)
#define menu_register(apid, string)				mt_menu_register(apid, string, &_globl)
#define menu_unregister(apid)						mt_menu_unregister(apid, &_globl)
#define menu_click(click, setit)					mt_menu_click(click, setit, &_globl)
#define menu_attach(flag, tree, item, mdata)	mt_menu_attach(flag, tree, item, mdata, &_globl)
#define menu_istart(flag, tree, imenu, item)	mt_menu_istart(flag, tree, imenu, item, &_globl)
#define menu_popup(menu, xpos, ypos, mdata)	mt_menu_popup(menu, xpos, ypos, mdata, &_globl)
#define menu_settings(flag, set)					mt_menu_settings(flag, set, &_globl)


/*-------------------------------------------------------------------
	Die OBJC-Funktionen
 -------------------------------------------------------------------*/

/* Objekt-Nummer */
#define G_BOX           20
#define G_TEXT          21
#define G_BOXTEXT       22
#define G_IMAGE         23
#define G_USERDEF       24
#define G_IBOX          25
#define G_BUTTON        26
#define G_BOXCHAR       27
#define G_STRING        28
#define G_FTEXT         29
#define G_FBOXTEXT      30
#define G_ICON          31
#define G_TITLE         32
#define G_CICON         33
#define G_SWBUTTON      34		/* MAG!X                				*/
#define G_POPUP         35		/* MAG!X                				*/
#define G_RESVD1	 		36		/* MagiC 3.1	 							*/
#define G_EDIT				37		/* MagiC 5.20, ber Shared Library	*/
#define G_SHORTCUT 		38		/* MagiC 6 									*/


/* Object flags */
#define NONE				0x0000
#define SELECTABLE		0x0001
#define DEFAULT			0x0002
#define EXIT				0x0004
#define EDITABLE			0x0008
#define RBUTTON			0x0010
#define LASTOB				0x0020
#define TOUCHEXIT			0x0040
#define HIDETREE			0x0080
#define INDIRECT			0x0100
#define FL3DIND			0x0200	/* 3D Indicator	AES 4.0 */
#define FL3DBAK			0x0400	/* 3D Background	AES 4.0 */
#define FL3DACT			0x0600	/* 3D Activator	AES 4.0 */
#define SUBMENU			0x0800

/* Objektstatus (OBJECT.ob_state) */
#define NORMAL				0x0000
#define SELECTED			0x0001
#define CROSSED			0x0002
#define CHECKED			0x0004
#define DISABLED			0x0008
#define OUTLINED			0x0010
#define SHADOWED			0x0020
#define WHITEBAK			0x0040		/* TOS         */
#define DRAW3D				0x0080		/* GEM 2.x     */


/* Object colors */
#if !defined(__COLORS)
#define __COLORS			/* Using AES-colors and BGI-colors is not possible */

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

/* Aus PC-GEM bernommen */
#define DWHITE				LWHITE
#define DBLACK				LBLACK
#define DRED				LREAD
#define DGREEN				LGREEN
#define DBLUE				LBLUE
#define DCYAN				LCYAN
#define DYELLOW			LYELLOW
#define DMAGENTA			LMAGENTA

#endif

/* Funktionsauswahl fr objc_edit */
#define ED_START			0
#define ED_INIT			1
#define ED_CHAR			2
#define ED_END				3
#define ED_CRSR			100	/* MAG!X       */
#define ED_DRAW			103	/* MAG!X 2.00  */


/* Spezial-Position fr objc_order */
#define OO_LAST	-1
#define OO_FIRST	0


/* Flag fr das Objektbaum-Zeichnen (fr objc_draw) */
#define NO_DRAW		0
#define REDRAW			1


/* Funktions-Codes fr objc_sysvar */
#define LK3DIND		1      /* AES 4.0    */
#define LK3DACT		2      /* AES 4.0    */
#define INDBUTCOL		3      /* AES 4.0    */
#define BACKGRCOL		4      /* AES 4.0    */
#define AD3DVAL		5      /* AES 4.0    */
#define AD3DVALUE    6      /* AES 4.0    */
#define MX_ENABLE3D 10      /* MagiC 3.0  */
#define MENUCOL     11      /* MagiC 6.0  */


/* Modus fr objc_sysvar */
#define SV_INQUIRE		0
#define SV_SET				1


short mt_objc_add( OBJECT *tree, const short parent, const short child,
				GlobalArray *globl );
short mt_objc_delete( OBJECT *tree, const short objnr, GlobalArray *globl );
short mt_objc_draw( OBJECT *tree, const short start, const short depth,
				const GRECT *clip, GlobalArray *globl );
short mt_objc_find( OBJECT *tree, const short start, const short depth,
				const short mx, const short my, GlobalArray *globl );
short mt_objc_offset( OBJECT *tree, const short objnr, short *x, short *y,
				GlobalArray *globl );
short mt_objc_order( OBJECT *tree, const short objnr, const short new_nr,
				GlobalArray *globl );
short mt_objc_edit( OBJECT *tree, const short objnr, const short inchar,
				short *idx, const short kind, GlobalArray *globl );
short mt_objc_xedit( OBJECT *tree, const short objnr, const short inchar, short *idx,
				short kind, GRECT *r, GlobalArray *globl );
short mt_objc_change( OBJECT *tree, const short objnr, const GRECT *clip,
				const short newstate, const short redraw, GlobalArray *globl );
short mt_objc_sysvar( const short mode, const short which, const short in1, const short in2,
				short *out1, short *out2, GlobalArray *globl );

#define objc_add(tree, parent, child)				mt_objc_add(tree, parent, child, &_globl)
#define objc_delete(tree, objnr)						mt_objc_delete(tree, objnr, &_globl)
#define objc_draw(tree, start, depth, clip) 		mt_objc_draw(tree, start, depth, clip, &_globl)
#define objc_find(tree, start, depth, mx, my)	mt_objc_find(tree, start, depth, mx, my, &_globl)
#define objc_offset(tree, objnr, x, y)				mt_objc_offset(tree, objnr, x, y, &_globl)
#define objc_order(tree, objnr, new_nr)			mt_objc_order(tree, objnr, new_nr, &_globl)
#define objc_edit(tree, objnr, inchar, idx, kind) \
																mt_objc_edit(tree, objnr, inchar, idx, kind, &_globl)
#define objc_xedit(tree, objnr, inchar, idx, kind, r) \
																mt_objc_xedit(tree, objnr, inchar, idx, kind, r, &_globl)
#define objc_change(tree, objnr, resvd, clip, newstate, redraw) \
																mt_objc_change(tree, objnr, clip, newstate, redraw, &_globl)
#define objc_sysvar(mode, which, in1, in2, out1, out2) \
																mt_objc_sysvar(mode, which, in1, in2, out1, out2, &_globl)

/* OBJC-Erweiterungen von MagiC */

void mt_objc_wdraw( OBJECT *tree, const short start, const short depth, GRECT *clip,
				const short whandle, GlobalArray *globl );
short mt_objc_wedit( OBJECT *tree, const short objnr, const short inchar, short *idx,
				const short kind, const short whandle, GlobalArray *globl );
void mt_objc_wchange( OBJECT *tree, const short objnr, const short newstate,
				GRECT *clip, const short whandle, GlobalArray *globl );
short mt_objc_xfind( OBJECT *obj, const short start, const short depth,
				const short x, const short y, GlobalArray *globl );

#define objc_wdraw(tree, start, depth, clip, whandle) \
																mt_objc_wdraw(tree, start, depth, clip, whandle, &_globl)
#define objc_wedit(tree, objnr, inchar, idx, kind, whandle) \
																mt_objc_wedit(tree, objnr, inchar, idx, kind, whandle, &_globl)
#define objc_wchange(tree, objnr, newstate, clip, whandle) \
																mt_objc_wchange(tree, objnr, newstate, clip, whandle, &_globl)
#define objc_xfind(obj, start, depth, x, y)		mt_objc_xfind(obj, start, depth, x, y, &_globl)


/*-------------------------------------------------------------------
	Die FORM-Funktionen
 -------------------------------------------------------------------*/

/* Typen fr form_dial */
#define FMD_START		0
#define FMD_GROW		1
#define FMD_SHRINK	2
#define FMD_FINISH	3


/* Fehlernummern (fr form_error) */
#define FERR_FILENOTFOUND		2
#define FERR_PATHNOTFOUND		3
#define FERR_NOHANDLES			4
#define FERR_ACCESSDENIED		5
#define FERR_LOWMEM				8
#define FERR_BADENVIRON			10
#define FERR_BADFORMAT			11
#define FERR_BADDRIVE			15
#define FERR_DELETEDIR			16
#define FERR_NOFILES				18


short mt_form_do( OBJECT *tree, const short start, GlobalArray *globl );
short mt_form_dial( const short flag, const GRECT *little, const GRECT *big, GlobalArray *globl );
short mt_form_alert( const short defbutton, char *string, GlobalArray *globl );
short mt_form_error( const short errnum, GlobalArray *globl );
short mt_form_center( OBJECT *tree, GRECT *center, GlobalArray *globl );
short mt_form_keybd( OBJECT *tree, const short obj, const short next,
				const short input_char, short *next_obj, short *pchar, GlobalArray *globl );
short mt_form_button( OBJECT *tree, const short obj, const short clicks, short *next_obj,
				GlobalArray *globl );
short mt_form_popup( OBJECT *tree, const short x, const short y, GlobalArray *globl );

#define form_do(tree, start)				mt_form_do(tree, start, &_globl)
#define form_dial(flag, little, big)	mt_form_dial(flag, little, big, &_globl)
#define form_alert(defbutton, string)	mt_form_alert(defbutton, string, &_globl)
#define form_error(errnum)					mt_form_error(errnum, &_globl)
#define form_center(tree, center)		mt_form_center(tree, center, &_globl)
#define form_keybd(tree, obj, next, input_char, next_obj, pchar) \
													mt_form_keybd(tree, obj, next, input_char, next_obj, pchar, &_globl)
#define form_button(tree, obj, clicks, next_obj) \
													mt_form_button(tree, obj, clicks, next_obj, &_globl)
#define form_popup(tree, x, y)			mt_form_popup(tree, x, y, &_globl)



/* FlyDials-Funktionen von MagiC */

/* Zuordnung Scancode -> Objektnummer mit Anzahl Klicks der FlyDials unter MagiC */
typedef struct
{
	char	scancode;
	char	nclicks;
	short	objnr;
} SCANX;

/* Scancode-Tabellen der FlyDials unter MagiC */
typedef struct
{
	SCANX *unsh;	/* Tabellen fr UnShift-Kombinationen   */
	SCANX *shift;	/* Tabellen fr Shift-Kombinationen     */
	SCANX *ctrl;	/* Tabellen fr Control-Kombinationen   */
	SCANX *alt;		/* Tabellen fr Alternate-Kombinationen */
	void  *resvd;	/* reserviert */
} XDO_INF;


short mt_form_wbutton( const OBJECT *fo_btree, const short fo_bobject,
				const short fo_bclicks, short *fo_bnxtobj, const short whandle,
				GlobalArray *globl );
short mt_form_xdial( const short fo_diflag, const GRECT *little, const GRECT *big,
				void **flydial, GlobalArray *globl );
short mt_form_xdo( const OBJECT *tree, const short startob, short *lastcrsr,
				const XDO_INF *tabs, void *flydial, GlobalArray *globl );
short mt_form_xerr( const long errcode, unsigned char *errfile, GlobalArray *globl );

#define form_wbutton(fo_btree, fo_bobject, fo_bclicks, fo_bnxtobj, whandle) \
													mt_form_wbutton(fo_btree, fo_bobject, fo_bclicks, fo_bnxtobj, whandle, &_globl)
#define form_xdial(flag, little, big, flydial) \
													mt_form_xdial(flag, little, big, flydial, &_globl)
#define form_xdo(tree, startob, lastcrsr, tabs, flydial) \
													mt_form_xdo(tree, startob, lastcrsr, tabs, flydial, &_globl)
#define form_xerr(errcode, errfile)		mt_form_xerr(errcode, errfile, &_globl)


/*-------------------------------------------------------------------
	Die GRAF-Funktionen
 -------------------------------------------------------------------*/

/* Struktur fr de Mausform */
typedef struct mfstr
{
	short mf_xhot;
	short mf_yhot;
	short mf_nplanes;
	short mf_fg;
	short mf_bg;
	short mf_mask[16];
	short mf_data[16];
} MFORM;

/* Maus-Formen */
#define ARROW				0
#define TEXT_CRSR			1
#define BUSYBEE			2
#define POINT_HAND		3
#define FLAT_HAND			4
#define THIN_CROSS		5
#define THICK_CROSS		6
#define OUTLN_CROSS		7
#define USER_DEF			255
#define M_OFF				256
#define M_ON				257
#define M_SAVE				258
#define M_LAST				259
#define M_RESTORE			260
#define M_FORCE			0x8000


short mt_graf_rubbox( const short x, const short y, const short w_min,
				const short h_min, short *w_end, short *h_end, GlobalArray *globl );
short mt_graf_dragbox( const short w, const short h, const short sx, const short sy,
				const GRECT *bound, short *x, short *y, GlobalArray *globl );
short mt_graf_mbox( const short w, const short h, short const start_x,	const short start_y,
				const short ende_x, const short ende_y, GlobalArray *globl );
short mt_graf_growbox( const GRECT *start, const GRECT *end, GlobalArray *globl );
short mt_graf_shrinkbox( const GRECT *start, const GRECT *end, GlobalArray *globl );
short mt_graf_watchbox( OBJECT *tree, const short obj_nr, const short instate,
				const short outstate, GlobalArray *globl );
short mt_graf_slidebox( OBJECT *tree, const short parent, const short obj_nr,
				const short isvert, GlobalArray *globl );
short mt_graf_handle( short *wchar, short *hchar, short *wbox, short *hbox,
				GlobalArray *globl );
short mt_graf_mouse( const short mouse_nr, MFORM *form, GlobalArray *globl );
short mt_graf_mkstate( EVNTDATA *ev, GlobalArray *globl );
short mt_graf_multirubber( const short x, const short y, const short minw, const short minh,
				GRECT *rec, short *outw, short *outh, GlobalArray *globl );

#define graf_rubbox(x, y, w_min, h_min, w_end, h_end) \
													mt_graf_rubbox( x, y, w_min, h_min, w_end, h_end, &_globl)
#define graf_dragbox(w, h, sx, sy, bound, x, y) \
													mt_graf_dragbox(w, h, sx, sy, bound, x, y, &_globl)
#define graf_mbox(w, h, start_x, start_y, ende_x, ende_y) \
													mt_graf_mbox(w, h, start_x, start_y, ende_x, ende_y, &_globl)
#define graf_movebox(w, h, start_x, start_y, ende_x, ende_y) \
													mt_graf_mbox(w, h, start_x, start_y, ende_x, ende_y, &_globl)
#define graf_growbox(start, end) 		mt_graf_growbox(start, end, &_globl)
#define graf_shrinkbox(start, end) 		mt_graf_shrinkbox(start, end, &_globl)
#define graf_watchbox(tree, obj_nr, instate, outstate) \
													mt_graf_watchbox(tree, obj_nr, instate, outstate, &_globl)
#define graf_slidebox(tree, parent, obj_nr, isvert) \
													mt_graf_slidebox(tree, parent, obj_nr, isvert, &_globl)
#define graf_handle(wchar, hchar, wbox, hbox) \
													mt_graf_handle(wchar, hchar, wbox, hbox, &_globl)
#define graf_mouse(mouse_nr, form)		mt_graf_mouse(mouse_nr, form, &_globl)
#define graf_mkstate(ev) 					mt_graf_mkstate(ev, &_globl)
#define graf_multirubber(x, y, minw, minh, rec, outw, outh) \
													mt_graf_multirubber(x, y, minw, minh, rec, outw, outh, &_globl)


/* GRAF-Erweiterungen von MagiC */

short mt_graf_wwatchbox( const OBJECT *gr_wptree, const short gr_wobject,
				const short gr_winstate, const short gr_woutstate, const short whandle,
				GlobalArray *globl );
short mt_graf_xhandle( short *wchar, short *hchar, short *wbox, short *hbox, short *device,
				GlobalArray *globl );

#define graf_wwatchbox(wptree, wobject, winstate, woutstate, whandle) \
									mt_graf_wwatchbox(wptree, wobject, winstate, woutstate, whandle, &_globl)
#define graf_xhandle(wchar, hchar, wbox, hbox, device) \
									mt_graf_xhandle(wchar, hchar, wbox, hbox, device, &_globl)



/*-------------------------------------------------------------------
	Die SCRP-Funktionen
 -------------------------------------------------------------------*/

short mt_scrp_read( char *pathname, GlobalArray *globl );
short mt_scrp_write( char *pathname, GlobalArray *globl );
short mt_scrp_clear( GlobalArray *globl );

#define scrp_read(pathname)			mt_scrp_read(pathname, &_globl)
#define scrp_write(pathname)			mt_scrp_write(pathname, &_globl)
#define scrp_clear()						mt_scrp_clear(&_globl)


/*-------------------------------------------------------------------
	Die FSEL-Funktionen
 -------------------------------------------------------------------*/

/* Rckgabewerte fr fsel_input und fsel_exinput */
#define FSEL_CANCEL		 0
#define FSEL_OK			 1

short mt_fsel_input( char *path, char *name, short *button, GlobalArray *globl );
short mt_fsel_exinput( char *path, char *name, short *button, const char *label,
				GlobalArray *globl );

#define fsel_input(path, name, button)					mt_fsel_input(path, name, button, &_globl)
#define fsel_exinput(path, name, button, label)		mt_fsel_exinput(path, name, button, label, &_globl)


/*-------------------------------------------------------------------
	Die WIND-Funktionen
 -------------------------------------------------------------------*/

/* Fensterkomponenten (fr wind_create) */
#define NAME			0x0001
#define CLOSER			0x0002
#define FULLER			0x0004
#define MOVER			0x0008
#define INFO			0x0010
#define SIZER			0x0020
#define UPARROW		0x0040
#define DNARROW		0x0080
#define VSLIDE			0x0100
#define LFARROW		0x0200
#define RTARROW		0x0400
#define HSLIDE			0x0800
#define HOTCLOSEBOX	0x1000		/* KaOS, MagiC */
#define MENUBAR		0x1000		/* XaAES */
#define BACKDROP		0x2000
#define ICONIFIER		0x4000

#define SMALLER		ICONIFIER


/* Die Infos fr wind_get und wind_set */
#define WF_KIND				    1
#define WF_NAME				    2
#define WF_INFO				    3
#define WF_WORKXYWH			    4
#define WF_CURRXYWH			    5
#define WF_PREVXYWH			    6
#define WF_FULLXYWH			    7
#define WF_HSLIDE				    8
#define WF_VSLIDE				    9
#define WF_TOP					   10
#define WF_FIRSTXYWH			   11
#define WF_NEXTXYWH			   12
#define WF_RESVD				   13
#define WF_NEWDESK			   14
#define WF_HSLSIZE			   15
#define WF_VSLSIZE			   16
#define WF_SCREEN				   17
#define WF_COLOR				   18
#define WF_DCOLOR				   19
#define WF_OWNER				   20
#define WF_BEVENT				   24
#define WF_BOTTOM				   25
#define WF_ICONIFY			   26
#define WF_UNICONIFY			   27
#define WF_UNICONIFYXYWH	   28
#define WF_TOOLBAR			   30
#define WF_FTOOLBAR			   31
#define WF_NTOOLBAR			   32
#define WF_MENU					33		/* XaAES */
#define WF_WHEEL					40		/* XaAES */
#define WF_M_BACKDROP    	  100		/* KAOS 1.4	*/
#define WF_M_OWNER       	  101		/* KAOS 1.4	*/
#define WF_M_WINDLIST    	  102		/* KAOS 1.4	*/
#define WF_MINXYWH		     103		/* MagiC 6	*/
#define WF_INFOXYWH			  104		/* MagiC 6.10  */
#define WF_WIDGETS			  200		/* Positionen der V- und H-Sliderelemente. */
#define WF_WINX				22360		/* WINX		*/
#define WF_WINXCFG			22361		/* WINX		*/
#define WF_DDELAY				22362		/* WINX		*/
#define WF_SHADE		  		22365		/* WINX 2.3	*/
#define WF_STACK		  		22366		/* WINX 2.3	*/
#define WF_TOPALL		  		22367		/* WINX 2.3	*/
#define WF_BOTTOMALL			22368		/* WINX 2.3	*/
#define WF_XAAES				0x5841	/* XaAES: 'XA' */

#define WF_WXYWH			WF_WORKXYWH
#define WF_CXYWH			WF_CURRXYWH
#define WF_PXYWH			WF_PREVXYWH
#define WF_FXYWH			WF_FULLXYWH


/* wind_set(WF_BEVENT) */
#define BEVENT_WORK     0x0001    /* AES 4.0  */
#define BEVENT_INFO     0x0002    /* MagiC 6  */


/* Die Fenster-Komponenten (fr wind_set, Modus WF_COLOR und WF_DCOLOR) */
#define W_BOX            0
#define W_TITLE          1
#define W_CLOSER         2
#define W_NAME           3
#define W_FULLER         4
#define W_INFO           5
#define W_DATA           6
#define W_WORK           7
#define W_SIZER          8
#define W_VBAR           9
#define W_UPARROW       10
#define W_DNARROW       11
#define W_VSLIDE        12
#define W_VELEV         13
#define W_HBAR          14
#define W_LFARROW       15
#define W_RTARROW       16
#define W_HSLIDE        17
#define W_HELEV         18
#define W_SMALLER       19
#define W_BOTTOMER		20					/* MagiC 3 */
#define W_HIDER			30


/* Die Konstanten fr die Scroll-Nachrichten */
#define WA_UPPAGE 	0
#define WA_DNPAGE 	1
#define WA_UPLINE 	2
#define WA_DNLINE 	3
#define WA_LFPAGE 	4
#define WA_RTPAGE 	5
#define WA_LFLINE 	6
#define WA_RTLINE 	7
#define WA_WHEEL		8	/* XaAES */


/* Flags fr wind_calc */
#define WC_BORDER	0
#define WC_WORK	1


/* Modi fr wind_update */
#define BEG_UPDATE		0x0001
#define END_UPDATE		0x0000
#define BEG_MCTRL			0x0003
#define END_MCTRL			0x0002


short mt_wind_create( const short kind, const GRECT *max, GlobalArray *globl );
short mt_wind_open( const short handle, const GRECT *rect, GlobalArray *globl );
short mt_wind_close( const short handle, GlobalArray *globl );
short mt_wind_delete( const short handle, GlobalArray *globl );
short mt_wind_get( const short handle, const short what, short *out1, short *out2,
				short *out3, short *out4, GlobalArray *globl );
short mt_wind_get_grect( const short handle, const short what,
				GRECT *r, GlobalArray *globl );
short mt_wind_get_ptr( const short handle, const short what,
				void **ptr, GlobalArray *globl );
short mt_wind_get_int( const short handle, const short what,
				short *out, GlobalArray *globl );
short mt_wind_set( const short handle, const short what, const short val1, const short val2,
				const short val3, const short val4, GlobalArray *globl );
short mt_wind_set_grect( const short handle, const short what, const GRECT *xywh,
				GlobalArray *globl );
short mt_wind_set_ptr( const short handle, const short what, const void *ptr,
				GlobalArray *globl );
short mt_wind_set_int( const short handle, const short what, const short value,
				GlobalArray *globl );
short mt_wind_find( const short x, const short y, GlobalArray *globl );
short mt_wind_update( const short what, GlobalArray *globl );
short mt_wind_calc( const short wtype, const short kind, const GRECT *input,
				GRECT *output, GlobalArray *globl );
short mt_wind_new( GlobalArray *globl );
short mt_wind_draw( const short handle, const short startob, GlobalArray *globl );

#define wind_create(kind, max)					mt_wind_create(kind, max, &_globl)
#define wind_open(handle, rect)					mt_wind_open(handle, rect, &_globl)
#define wind_close(handle)							mt_wind_close(handle, &_globl)
#define wind_delete(handle)						mt_wind_delete(handle, &_globl)
#define wind_get(handle, what, o1, o2, o3, o4)	\
															mt_wind_get(handle, what, o1, o2, o3, o4, &_globl)
#define wind_get_grect(handle, what, g)		mt_wind_get_grect(handle, what, g, &_globl)
#define wind_get_ptr(handle, what, ptr)		mt_wind_get_ptr(handle, what, ptr, &_globl)
#define wind_get_int(handle, what, o1)			mt_wind_get_int(handle, what, o1, &_globl)
#define wind_set(handle, what, o1, o2, o3, o4)	\
															mt_wind_set(handle, what, o1, o2, o3, o4, &_globl)
#define wind_set_grect(handle, what, xywh)	mt_wind_set_grect(handle, what, xywh, &_globl)
#define wind_set_ptr(handle, what, ptr)		mt_wind_set_ptr(handle, what, ptr, &_globl)
#define wind_set_string(handle, what, ptr)	mt_wind_set_ptr(handle, what, ptr, &_globl)
#define wind_set_int(handle, what, val)		mt_wind_set_int(handle, what, val, &_globl)
#define wind_find(x, y)								mt_wind_find(x, y, &_globl)
#define wind_update(what)							mt_wind_update(what, &_globl)
#define wind_calc(wtype, kind, input, output) \
															mt_wind_calc(wtype, kind, input, output, &_globl)
#define wind_new()									mt_wind_new(&_globl)
#define wind_draw(handle, startob)				mt_wind_draw(handle, startob, &_globl)


/*-------------------------------------------------------------------
	Die RSRC-Funktionen
 -------------------------------------------------------------------*/

/* Header einer RSC-Datei */
typedef struct rshdr
{
	 rsh_vrsn;
	 rsh_object;
	 rsh_tedinfo;
	 rsh_iconblk;    /* list of ICONBLKS */
	 rsh_bitblk;
	 rsh_frstr;
	 rsh_string;
	 rsh_imdata;     /* image data */
	 rsh_frimg;
	 rsh_trindex;
	 rsh_nobs;       /* counts of various structs */
	 rsh_ntree;
	 rsh_nted;
	 rsh_nib;
	 rsh_nbb;
	 rsh_nstring;
	 rsh_nimages;
	 rsh_rssize;     /* total bytes in resource */
} RSHDR;


/* Die Struktur-Typen (fr rsrc_gaddr) */
#define R_TREE				0
#define R_OBJECT			1
#define R_TEDINFO			2
#define R_ICONBLK			3
#define R_BITBLK			4
#define R_STRING			5
#define R_IMAGEDATA		6
#define R_OBSPEC			7
#define R_TEPTEXT			8
#define R_TEPTMPLT		9
#define R_TEPVALID		10
#define R_IBPMASK			11
#define R_IBPDATA			12
#define R_IBPTEXT       13
#define R_IPBTEXT       R_IBPTEXT
#define R_BIPDATA			14
#define R_FRSTR			15
#define R_FRIMG			16


#define ROOT             0					/* the root object of a tree   */
/*#define NIL             -1				 nil object index            */
#define MAX_LEN         81					/* max string length           */
#define MAX_DEPTH        8					/* max depth of search or draw */


/* Zeichensatz-Typen */
#define GDOS_PROP			0
#define GDOS_MONO			1
#define GDOS_BITM			2
#define IBM             3
#define SMALL           5


/* Ausrichtung eines editierbaren Textfeldes */
#define TE_LEFT	0
#define TE_RIGHT	1
#define TE_CNTR	2


short mt_rsrc_load( char *name, GlobalArray *globl );
short mt_rsrc_free( GlobalArray *globl );
short mt_rsrc_gaddr( const short type, const short id, void *addr, GlobalArray *globl );
short mt_rsrc_saddr( const short type, const short id, void *addr, GlobalArray *globl );
short mt_rsrc_obfix( OBJECT *tree, const short obj, GlobalArray *globl );
short mt_rsrc_rcfix( void *rc_header, GlobalArray *globl );

#define rsrc_load(name)						mt_rsrc_load(name, &_globl)
#define rsrc_free()							mt_rsrc_free(&_globl)
#define rsrc_gaddr(type, id, addr)		mt_rsrc_gaddr(type, id, addr, &_globl)
#define rsrc_saddr(type, id, addr)		mt_rsrc_saddr(type, id, addr, &_globl)
#define rsrc_obfix(tree, obj)				mt_rsrc_obfix(tree, obj, &_globl)
#define rsrc_rcfix(rc_header)				mt_rsrc_rcfix(rc_header, &_globl)


/*-------------------------------------------------------------------
	Die SHEL-Funktionen
 -------------------------------------------------------------------*/

/* Struktur fr erweiterte Start-Daten (fr shel_write) */
typedef struct
{
	char *newcmd;
	long psetlimit;
	long prenice;
	char *defdir;
	char *env;
} SHELW;

/* Andere Definition dafr (aus MagiC-Doku) */
typedef struct
{
	char	*command;
	long	limit;
	long	nice;
	char	*defdir;
	char	*env;
} XSHW_COMMAND;


typedef struct
{
	long	cdecl (*proc)(void *par);
	void		*user_stack;
	unsigned long	stacksize;
	short		mode;		/* immer auf 0 setzen! */
	long		res1;		/* immer auf 0L setzen! */
} THREADINFO;


/* tail for default shell */
typedef struct
{
	short	dummy;                   /* ein Nullwort               */
	long	magic;                   /* 'SHEL', wenn ist Shell     */
	short	isfirst;                 /* erster Aufruf der Shell    */
	long	lasterr;                 /* letzter Fehler             */
	short	wasgr;                   /* Programm war Grafikapp.    */
} SHELTAIL;


/* Modus fr shel_get */
#define SHEL_BUFSIZE (-1)


/* shel_write modes for parameter "doex" */
#define SHW_NOEXEC			0
#define SHW_EXEC				1
#define SHW_EXEC_ACC			3		/* AES 3.3	*/
#define SHW_SHUTDOWN			4		/* AES 3.3     */
#define SHW_RESCHNG			5		/* AES 3.3     */
#define SHW_BROADCAST		7		/* AES 4.0     */
#define SHW_INFRECGN			9		/* AES 4.0     */
#define SHW_AESSEND			10    /* AES 4.0     */
#define SHW_THR_CREATE		20		/* MagiC 4.5	*/
#define SHW_XMDLIMIT			256
#define SHW_XMDNICE			512
#define SHW_XMDDEFDIR		1024
#define SHW_XMDENV			2048

/* Konstantennamen aus der GEMLIB */
#define SWM_LAUNCH		SHW_NOEXEC
#define SWM_LAUNCHNOW	SHW_EXEC
#define SWM_LAUNCHACC	SHW_EXEC_ACC
#define SWM_SHUTDOWN		SHW_SHUTDOWN
#define SWM_REZCHANGE	SHW_RESCHNG
#define SWW_BROADCAST	SHW_BROADCAST
#define SWM_ENVIRON		8
#define SWM_NEWMSG		SHW_INFRECGN
#define SWM_AESMSG		SHW_AESSEND


/* shel_write modes for parameter "isover" */
#define SHW_IMMED			0                                  /* PC-GEM 2.x  */
#define SHW_CHAIN			1                                  /* TOS         */
#define SHW_DOS			2                                  /* PC-GEM 2.x  */
#define SHW_RESCHANGE	5
#define SHW_PARALLEL		100                                /* MAG!X       */
#define SHW_SINGLE		101                                /* MAG!X       */
#define SHW_XMDFLAGS		4096


/* Erweiterte Start-Flags fr die Modi SWM_LAUNCH, SWM_LAUNCHNOW */
/* und SWM_LAUNCHACC (fr shel_write)                            */
#define SW_PSETLIMIT		0x0100
#define SW_PRENICE		0x0200
#define SW_PDEFDIR		0x0400
#define SW_ENVIRON		0x0800


/*  */
#define CL_NORMAL			0
#define CL_PARSE			1


/* Werte fr Modus SHW_SHUTDOWN bzw. SWM_SHUTDOWN (fr shel_write) */
#define SD_ABORT		0
#define SD_PARTIAL	1
#define SD_COMPLETE	2


/* Werte fr Modus SWM_ENVIRON (fr shel_write) */
#define ENVIRON_SIZE		0
#define ENVIRON_CHANGE	1
#define ENVIRON_COPY		2


/* Werte fr Modus SHW_INFRECGN bzw. SWM_NEWMSG (fr shel_write) */
#define NM_APTERM		0x0001


/* Werte fr Modus SHW_AESSEND bzw. SWM_AESMSG (fr shel_write) */
#define AP_AESTERM	52     /* Mode 10: N.AES komplett terminieren. */


/* Modus fr shel_help */
#define SHP_HELP	0


short mt_shel_read( char *cmd, char *tail, GlobalArray *globl );
short mt_shel_write( const short do_execute, const short is_graph, const short is_overlay,
				char *cmd, char *tail, GlobalArray *globl );
short mt_shel_get( char *addr, const short len, GlobalArray *globl );
short mt_shel_put( char *addr, const short len, GlobalArray *globl );
short mt_shel_find( char *path, GlobalArray *globl );
short mt_shel_envrn( char **value, char *name, GlobalArray *globl );
short mt_shel_rdef( char *cmd, char *dir, GlobalArray *globl );
short mt_shel_wdef( char *cmd, char *dir, GlobalArray *globl );
short mt_shel_help( const short sh_hmode, char *sh_hfile, char *sh_hkey,
				GlobalArray *globl );

#define shel_read(cmd, tail)				mt_shel_read(cmd, tail, &_globl)
#define shel_write(do_execute, is_graph, is_overlay, cmd, tail) \
													mt_shel_write(do_execute, is_graph, is_overlay, cmd, tail, &_globl)
#define shel_get(addr, len)				mt_shel_get(addr, len, &_globl)
#define shel_put(addr, len)				mt_shel_put(addr, len, &_globl)
#define shel_find(path)						mt_shel_find(path, &_globl)
#define shel_envrn(value, name)			mt_shel_envrn(value, name, &_globl)
#define shel_environ(value, name)		mt_shel_envrn(value, name, &_globl)
#define shel_rdef(cmd, dir)				mt_shel_rdef(cmd, dir, &_globl)
#define shel_wdef(cmd, dir)				mt_shel_wdef(cmd, dir, &_globl)
#define shel_help(mode, file, key)		mt_shel_help(mode, file, key, &_globl)


/*-------------------------------------------------------------------
	Die FSLX-Funktionen (MagiC, WDialog)
 -------------------------------------------------------------------*/

/* Die Filter-Funktion fr fslx_open */
#ifndef __TOS
#define XATTR void
#endif
typedef short (cdecl XFSL_FILTER)(char *path, char *name, XATTR *xa);


/* Flags des File-Selector */
#define DOSMODE		1
#define NFOLLOWSLKS	2
#define GETMULTI		8


/* Sortierung beim File-Selector */
#define SORTBYNAME	0
#define SORTBYDATE	1
#define SORTBYSIZE	2
#define SORTBYTYPE	3
#define SORTBYNONE	4
#define SORTDEFAULT -1


/* Globale Flags des File-Selectors (fslx_set_flags) */
#define SHOW8P3	1


void *mt_fslx_open( char *title, const short x, const short y, short *handle, char *path,
				const short pathlen, char *fname, const short fnamelen, char *patterns,
				XFSL_FILTER *filter, char *paths, const short sort_mode, const short flags,
				GlobalArray *globl );
short mt_fslx_close( void *fsd, GlobalArray *globl );
short mt_fslx_getnxtfile( void *fsd, char *fname, GlobalArray *globl );
short mt_fslx_evnt( void *fsd, EVNT *events, char *path, char *fname, short *button,
			short *nfiles, short *sort_mode, char **pattern, GlobalArray *globl );
void *mt_fslx_do( char *title, char *path, const short pathlen, char *fname,
				const short fnamelen, char *patterns, XFSL_FILTER *filter, char *paths,
				short *sort_mode, const short flags, short *button, short *nfiles, char **pattern,
				GlobalArray *globl );
short mt_fslx_set_flags( const short flags, short *oldval, GlobalArray *globl );

#define fslx_open(a, b, c, d, e, f, g, h, i, j, k, l, m) \
														mt_fslx_open(a, b, c, d, e, f, g, h, i, j, k, l, m, &_globl)
#define fslx_close(fsd)							mt_fslx_close(fsd, &_globl)
#define fslx_getnxtfile(fsd, fname)			mt_fslx_getnxtfile(fsd, fname, &_globl)
#define fslx_evnt(a, b, c, d, e, f, g, h)	mt_fslx_evnt(a, b, c, d, e, f, g, h, &_globl)
#define fslx_do(a, b, c, d, e, f, g, h, i, j, k, l, m) \
														mt_fslx_do(a, b, c, d, e, f, g, h, i, j, k, l, m, &_globl)
#define fslx_set_flags(flags, oldval)		mt_fslx_set_flags(flags, oldval, &_globl)


/*-------------------------------------------------------------------
	Die FNTS-Funktionen (MagiC, WDialog)
 -------------------------------------------------------------------*/

/* Datentyp fr Vektorfonts ab NVDI 4.x */
#ifndef __VDI__
typedef long fix31;
#endif

/* Fr die FNTS-Funktionen */

typedef void *FNT_DIALOG;

typedef void (cdecl *UTXT_FN)(short x, short y, short *clip_rect,
							long id, long pt, long ratio, char *string);

typedef struct _fnts_item
{
	struct	_fnts_item  *next;	/* Zeiger auf den nchsten Font oder 0L */
	UTXT_FN	display;					/* Anzeige-Funktion fr eigene Fonts    */
	long		id;						/* ID des Fonts                         */
	short		index;					/* mu 0 sein, da kein VDI-Font         */
	char		mono;						/* Flag fr quidistante Fonts          */
	char		outline;					/* Flag fr Vektorfont                  */
	short		npts;						/* Anzahl der vordefinierten Punkthhen */
	char		*full_name;				/* Zeiger auf den vollstndigen Namen   */
	char		*family_name;			/* Zeiger auf den Familiennamen         */
	char		*style_name;			/* Zeiger auf den Stilnamen             */
	char		*pts;						/* Zeiger auf Feld mit Punkthhen       */
	long		reserved[4];			/* reserviert, mssen 0 sein            */
} FNTS_ITEM;

/* Globale Flags des Font-Dialoges (fnts_create) */
#define FNTS_3D	1


/* Art der zugelassenen Zeichenstze (fnts_create) */
#define FNTS_BTMP		0x01	/* Bitmap-Fonts anzeigen */
#define FNTS_OUTL		0x02	/* Vektor-Fonts anzeigen */
#define FNTS_MONO		0x04	/* quidistante Fonts anzeigen */
#define FNTS_PROP		0x08	/* Proportionale Fonts anzeigen */


/* Anzeige der Buttons im Dialog (fnts_open, fnts_do) */
#define FNTS_SNAME		0x0001	/* Checkbox fr Name selektieren */
#define FNTS_SSTYLE		0x0002	/* Checkbox fr Stil selektieren */
#define FNTS_SSIZE		0x0004	/* Checkbox fr Gre selektieren */
#define FNTS_SRATIO		0x0008	/* Checkbox fr Verhltnis selektieren */
#define FNTS_CHNAME		0x0100	/* Checkbox fr Namen anzeigen */
#define FNTS_CHSTYLE		0x0200	/* Checkbox fr Stil anzeigen */
#define FNTS_CHSIZE		0x0400	/* Checkbox fr Gre anzeigen */
#define FNTS_CHRATIO		0x0800	/* Checkbox fr Verhltnis anzeigen */
#define FNTS_RATIO		0x1000	/* Verhltnis Breite/Hhe einstellbar */
#define FNTS_BSET			0x2000	/* Button "Setzen" anwhlbar */
#define FNTS_BMARK		0x4000	/* Button "Markieren" anwhlbar */


/* Konstanten fr die Buttons im fnts-Dialog */
#define FNTS_CANCEL	1
#define FNTS_OK		2
#define FNTS_SET		3
#define FNTS_MARK		4
#define FNTS_OPTION	5

/* Konstante aus GEMLIB */
#define FNTS_OPT		FNTS_OPTION

short mt_fnts_add( const FNT_DIALOG *fnt_dialog, const FNTS_ITEM *user_fonts,
				GlobalArray *globl );
short mt_fnts_close( const FNT_DIALOG *fnt_dialog, short *x, short *y,
				GlobalArray *globl );
FNT_DIALOG *mt_fnts_create( const short vdi_handle, const short no_fonts,
				const short font_flags, const short dialog_flags,
				const char *sample, const char *opt_button, GlobalArray *globl );
short mt_fnts_delete( const FNT_DIALOG *fnt_dialog, const short vdi_handle,
				GlobalArray *globl );
short mt_fnts_do( FNT_DIALOG *fnt_dialog, const short button_flags, const long id_in,
				const fix31 pt_in, const long ratio_in, short *check_boxes,
				long *id, fix31 *pt, fix31 *ratio, GlobalArray *globl );
short mt_fnts_evnt( FNT_DIALOG *fnt_dialog, EVNT *events, short *button,
				short *check_boxes, long *id, fix31 *pt, fix31 *ratio, GlobalArray *globl );
short mt_fnts_get_info( FNT_DIALOG *fnt_dialog, const long id, short *mono,
				short *outline, GlobalArray *globl );
short mt_fnts_get_name( FNT_DIALOG *fnt_dialog, const long id, char *full_name,
				char *family_name, char *style_name, GlobalArray *globl );
short mt_fnts_get_no_styles( FNT_DIALOG *fnt_dialog, const long id, GlobalArray *globl );
long mt_fnts_get_style( FNT_DIALOG *fnt_dialog, const long id, const short index,
				GlobalArray *globl );
short mt_fnts_open( FNT_DIALOG *fnt_dialog, const short button_flags, const short x,
				const short y, const long id, const fix31 pt, const fix31 ratio,
				GlobalArray *globl );
void mt_fnts_remove( FNT_DIALOG *fnt_dialog, GlobalArray *globl );
short mt_fnts_update( FNT_DIALOG *fnt_dialog, const short button_flags, const long id,
				const fix31 pt, const long ratio, GlobalArray *globl );

#define fnts_add(fnt_dialog, user_fonts)		mt_fnts_add(fnt_dialog, user_fonts, &_globl)
#define fnts_close(fnt_dialog, x, y)			mt_fnts_close(fnt_dialog, x, y, &_globl)
#define fnts_create(a, b, c, d, e, f)			mt_fnts_create(a, b, c, d, e, f, &_globl)
#define fnts_delete(fnt_dialog, vdi_handle)	mt_fnts_delete(fnt_dialog, vdi_handle, &_globl)
#define fnts_do(a, b, c, d, e, f, g, h, i)	mt_fnts_do(a, b, c, d, e, f, g, h, i, &_globl)
#define fnts_evnt(a, b, c, d, e, f, g)			mt_fnts_evnt(a, b, c, d, e, f, g, &_globl)
#define fnts_get_info(fnt_dialog, id, mono, outline) \
															mt_fnts_get_info(fnt_dialog, id, mono, outline, &_globl)
#define fnts_get_name(fnt_dialog, id, full_name, family_name, style_name) \
															mt_fnts_get_name(fnt_dialog, id, full_name, \
																	family_name, style_name, &_globl)
#define fnts_get_no_styles(fnt_dialog, id)	mt_fnts_get_no_styles(fnt_dialog, id, &_globl)
#define fnts_get_style(fnt_dialog, id, index)mt_fnts_get_style(fnt_dialog, id, index, &_globl)
#define fnts_open(fnt_dialog, button_flags, x, y, id, pt, ratio) \
															mt_fnts_open(fnt_dialog, button_flags, x, y, id, pt, ratio, &_globl)
#define fnts_remove(fnt_dialog)					mt_fnts_remove(fnt_dialog, &_globl)
#define fnts_update(fnt_dialog, button_flags, id, pt, ratio) \
															mt_fnts_update(fnt_dialog, button_flags, id, pt, ratio, &_globl)

/*-------------------------------------------------------------------
	Die LBOX-Funktionen (MagiC, WDialog)
 -------------------------------------------------------------------*/

typedef void *DIALOG;

typedef void *LIST_BOX;

typedef struct _lbox_item
{
    struct _lbox_item *next;  /* Zeiger auf den nchsten Eintrag */
                              /* in der Scroll-Liste             */

    short	selected;         /* Objekt selektiert?     */
    short	data1;            /* Daten fr das Programm */
    void		*data2;
    void		*data3;

} LBOX_ITEM;


typedef void (cdecl *SLCT_ITEM)( LIST_BOX *box, OBJECT *tree,
                                 LBOX_ITEM *item,
                                 void *user_data, short obj_index,
                                 short last_state );

typedef short (cdecl *SET_ITEM)( LIST_BOX *box, OBJECT *tree,
                                 LBOX_ITEM *item,
                                 short obj_index, void *user_data,
                                 GRECT *rect, short first );


/* Konstanten fr die Listboxen (lbox_...) */
#define LBOX_VERT			1				/* Listbox mit vertikalem Slider								*/
#define LBOX_AUTO			2				/* Auto-Scrolling													*/
#define LBOX_AUTOSLCT	4				/* automatische Darstellung beim Auto-Scrolling			*/
#define LBOX_REAL			8				/* Real-Time-Slider												*/
#define LBOX_SNGL			16				/* nur ein anwhlbarer Eintrag								*/
#define LBOX_SHFT			32				/* Mehrfachselektionen mit Shift								*/
#define LBOX_TOGGLE		64				/* Status eines Eintrags bei Selektion wechseln			*/
#define LBOX_2SLDRS		128			/* Listbox hat einen hor. und einen vertikalen Slider	*/


void mt_lbox_ascroll_to( LIST_BOX *box, const short first, GRECT *box_rect,
				GRECT *slider_rect, GlobalArray *globl );
void mt_lbox_bscroll_to( LIST_BOX *box, const short first, GRECT *box_rect,
				GRECT *slider_rect, GlobalArray *globl );
short mt_lbox_cnt_items( LIST_BOX *box, GlobalArray *globl );
LIST_BOX *mt_lbox_create( OBJECT *tree, SLCT_ITEM slct, SET_ITEM set,
					LBOX_ITEM *items, const short visible_a, const short first_a,
					const short *ctrl_objs, const short *objs, const short flags,
					const short pause_a, void *user_data, DIALOG *dialog,
					const short visible_b, const short first_b, const short entries_b,
					const short pause_b, GlobalArray *globl );
short mt_lbox_delete( LIST_BOX *box, GlobalArray *globl );
short mt_lbox_do( LIST_BOX *box, const short obj, GlobalArray *globl );
void mt_lbox_free_items( LIST_BOX *box, GlobalArray *globl );
void mt_lbox_free_list( LBOX_ITEM *items, GlobalArray *globl );
short mt_lbox_get_afirst( LIST_BOX *box, GlobalArray *globl );
short mt_lbox_get_avis( LIST_BOX *box, GlobalArray *globl );
short mt_lbox_get_visible( LIST_BOX *box, GlobalArray *globl );
short mt_lbox_get_bentries( LIST_BOX *box, GlobalArray *globl );
short mt_lbox_get_bfirst( LIST_BOX *box, GlobalArray *globl );
short mt_lbox_get_bvis( LIST_BOX *box, GlobalArray *globl );
short mt_lbox_get_idx( LBOX_ITEM *items, LBOX_ITEM *search,GlobalArray *globl );
LBOX_ITEM *mt_lbox_get_item( LIST_BOX *box, const short n, GlobalArray *globl );
LBOX_ITEM *mt_lbox_get_items( LIST_BOX *box, GlobalArray *globl );
short mt_lbox_get_slct_idx( LIST_BOX *box, GlobalArray *globl );
LBOX_ITEM *mt_lbox_get_slct_item( LIST_BOX *box, GlobalArray *globl );
OBJECT *mt_lbox_get_tree( LIST_BOX *box, GlobalArray *globl );
void *mt_lbox_get_udata( LIST_BOX *box, GlobalArray *globl );
void mt_lbox_set_asldr( LIST_BOX *box, const short first, GRECT *rect, GlobalArray *globl );
void mt_lbox_set_slider( LIST_BOX *box, const short first, GRECT *rect, GlobalArray *globl );
void mt_lbox_set_bentries( LIST_BOX *box, const short entries, GlobalArray *globl );
void mt_lbox_set_bsldr( LIST_BOX *box, const short first, GRECT *rect, GlobalArray *globl );
void mt_lbox_set_items( LIST_BOX *box, LBOX_ITEM *items, GlobalArray *globl );
void mt_lbox_update( LIST_BOX *box, GRECT *rect, GlobalArray *globl );

#define lbox_ascroll_to(box, first, box_rect, slider_rect) \
												mt_lbox_ascroll_to(box, first, box_rect, slider_rect, &_globl)
#define lbox_scroll_to(box, first, box_rect, slider_rect) \
												mt_lbox_ascroll_to(box, first, box_rect, slider_rect, &_globl)
#define lbox_bscroll_to(box, first, box_rect, slider_rect) \
												mt_lbox_bscroll_to(box, first, box_rect, slider_rect, &_globl)
#define lbox_cnt_items(box)			mt_lbox_cnt_items(box, &_globl)
#define lbox_create(tr, slct, set, items, v_a, f_a, c_objs, objs, flags, p_a, user, dia, v_b, f_b, e_b, p_b) \
												mt_lbox_create(tr, slct, set, items, v_a, f_a, c_objs, objs, flags, p_a, user, dia, v_b, f_b, e_b, p_b, &_globl)
#define lbox_delete(box)				mt_lbox_delete(box, &_globl)
#define lbox_do(box, obj)				mt_lbox_do(box, obj, &_globl)
#define lbox_free_items(box)			mt_lbox_free_items(box, &_globl)
#define lbox_free_list(items)			mt_lbox_free_list(items, &_globl)
#define lbox_get_afirst(box)			mt_lbox_get_afirst(box, &_globl)
#define lbox_get_first(box)			mt_lbox_get_afirst(box, &_globl)
#define lbox_get_avis(box)				mt_lbox_get_avis(box, &_globl)
#define lbox_get_visible(box)			mt_lbox_get_visible(box, &_globl)
#define lbox_get_bentries(box)		mt_lbox_get_bentries(box, &_globl)
#define lbox_get_bfirst(box)			mt_lbox_get_bfirst(box, &_globl)
#define lbox_get_idx(items, search)	mt_lbox_get_idx(items, search, &_globl)
#define lbox_get_item(box, n)			mt_lbox_get_item(box, n, &_globl)
#define lbox_get_items(box)			mt_lbox_get_items(box, &_globl)
#define lbox_get_bvis(box)				mt_lbox_get_bvis(box, &_globl)
#define lbox_get_slct_idx(box)		mt_lbox_get_slct_idx(box, &_globl)
#define lbox_get_slct_item(box)		mt_lbox_get_slct_item(box, &_globl)
#define lbox_get_tree(box)				mt_lbox_get_tree(box, &_globl)
#define lbox_get_udata(box)			mt_lbox_get_udata(box, &_globl)
#define lbox_set_asldr(box, first, rect) \
												mt_lbox_set_asldr(box, first, rect, &_globl)
#define lbox_set_slider(box, first, rect) \
												mt_lbox_set_slider(box, first, rect, &_globl)
#define lbox_set_bentries(box, entries) \
												mt_lbox_set_bentries(box, entries, &_globl)
#define lbox_set_bsldr(box, first, rect) \
												mt_lbox_set_bsldr(box, first, rect, &_globl)
#define lbox_set_items(box, items)	mt_lbox_set_items(box, items, &_globl)
#define lbox_update(box, rect)		mt_lbox_update(box, rect, &_globl)


/*-------------------------------------------------------------------
	Die PDLG-Funktionen (MagiC, WDialog)
 -------------------------------------------------------------------*/

typedef void *PRN_DIALOG;

/* "Forward-Deklarationen" fr die Druckdialoge */
struct _pdlg_sub;
struct _prn_entry;
struct _prn_settings;
struct _drv_entry;


/* Typen der Funktionszeiger fr die Druckdialoge */
typedef long (cdecl *PRN_SWITCH)( struct _drv_entry *drivers,
                                   struct _prn_settings *settings,
                                   struct _prn_entry *old_printer,
                                   struct _prn_entry *new_printer );

typedef long (cdecl *PDLG_INIT)( struct _prn_settings *settings,
                                  struct _pdlg_sub *sub );

typedef long (cdecl *PDLG_HNDL)( struct _prn_settings *settings,
                                  struct _pdlg_sub *sub,
                                  short exit_obj );

typedef long (cdecl *PDLG_RESET)( struct _prn_settings *settings,
                                   struct _pdlg_sub *sub );



/* Wird fr die Druckdialoge von WDialog bentigt */
typedef struct _pdlg_sub
{
	struct _pdlg_sub *next;			/* Zeiger auf Nachfolger           */
	long			length;				/* Strukturlnge                   */
	long			format;				/* Datenformat                     */
	long			reserved;			/* reserviert                      */

	void			*drivers;			/* nur fr interne Dialoge         */
	short			option_flags;		/* verschiedene Flags              */
	short			sub_id;				/* Kennung des Unterdialogs        */
	DIALOG		*dialog;				/* Zeiger auf die Struktur des     */
											/* Fensterdialogs oder 0L          */

	OBJECT		*tree;				/* Zeiger auf den Objektbaum       */
	short			index_offset;		/* Offset des Unterdialogs         */
	short			reserved1;			/* reserviert                      */
	long			reserved2;			/* reserviert                      */
	long			reserved3;			/* reserviert                      */
	long			reserved4;			/* reserviert                      */

	PDLG_INIT	init_dlg;			/* Initialisierungsfunktion        */
	PDLG_HNDL	do_dlg;				/* Behandlungsfunktion             */
	PDLG_RESET	reset_dlg;			/* Zurcksetzfunktion              */
	long			reserved5;			/* reserviert                      */

	OBJECT      *sub_icon;			/* Zeiger auf das Icon der Listbox */
	OBJECT      *sub_tree;			/* Objektbaum des Unterdialogs     */
	long			reserved6;			/* reserviert                      */
	long			reserved7;			/* reserviert                      */

	long			private1;			/* dialogeigene Informationen-1    */
	long			private2;			/* dialogeigene Informationen-2    */
	long			private3;			/* dialogeigene Informationen-3    */
	long			private4;			/* dialogeigene Informationen-4    */
} PDLG_SUB;


/* Wird fr die Druckdialoge von WDialog bentigt */
typedef struct _media_type
{
	struct _media_type   *next;	/* Zeiger auf Nachfolger     */
	long			type_id;				/* Kennung des Papierformats */
	char			name[32];			/* Name des Papierformats    */
} MEDIA_TYPE;


/* Wird fr die Druckdialoge von WDialog bentigt */
typedef struct _media_size
{
	struct _media_size *next;		/* Zeiger auf Nachfolger     */
	long			size_id;				/* Kennung des Papierformats */
	char			name[32];			/* Name des Papierformats    */
} MEDIA_SIZE;


/* Typen der Funktionszeiger fr die Druckdialoge */
typedef struct _drv_entry
{
	struct _drv_entry *next;
} DRV_ENTRY;


/* Wird fr die Druckdialoge von WDialog bentigt */
typedef struct _prn_tray
{
	struct _prn_tray  *next;		/* Zeiger auf Nachfolger       */
	long			tray_id;				/* Nummer des Einzugs/Auswurfs */
	char			name[32];			/* Name des Schachts           */
} PRN_TRAY;


/* Wird fr die Druckdialoge von WDialog bentigt */
typedef struct _prn_mode
{
	struct _prn_mode *next;				/* Zeiger auf Nachfolger             */
	long			mode_id;					/* Modus (Index innerhalb der Datei) */
	short			hdpi;						/* horizontale Auflsung in dpi      */
	short			vdpi;						/* vertikale Auflsung in dpi        */
	long			mode_capabilities;	/* Moduseigenschaften                */

	long			color_capabilities;	/* einstellbare Farbmodi             */
	long			dither_flags;			/* Flags, die angeben, ob der        */
												/* korrespondierende Farbmodus mit   */
												/* oder ohne Dithern ansprechbar ist */

	MEDIA_TYPE	*paper_types;			/* geeignete Papiertypen             */
	long			reserved;				/* reserviert                        */
	char			name[32];				/* Modusname                         */
} PRN_MODE;


/* Wird fr die Druckdialoge von WDialog bentigt */
typedef struct _prn_entry
{
	struct _prn_entry *next;				/* Zeiger auf Nachfolger           */
	long			length;						/* Strukturlnge                   */
	long			format;						/* Datenformat                     */
	long			reserved;					/* reserviert                      */

	short			driver_id;					/* Treiberkennung                  */
	short			driver_type;				/* Treibertyp                      */
	long			printer_id;					/* Druckerkennung                  */
	long			printer_capabilities;	/* Druckereigenschaften            */
	long			reserved1;					/* reserviert                      */

	long			flags;						/* verschiedene Flags              */
	struct _pdlg_sub  *sub_dialogs;		/* Zeiger auf Unterdialoge         */
   PRN_SWITCH	setup_panel;				/* Unterdialog bei Druckerwechsel  */
   												/* initialisieren                  */
	PRN_SWITCH	close_panel;				/* Unterdialog bei Druckerwechsel  */
													/* schlieen                       */

	PRN_MODE		*modes;						/* Liste vorhand. Auflsungen      */
	MEDIA_SIZE	*papers;						/* Liste vorhand. Papierformate    */
	PRN_TRAY		*input_trays;				/* Liste der Einzge               */
	PRN_TRAY		*output_trays;				/* Liste der Auswrfe              */
	char			name[32];					/* Name des Druckers               */
} PRN_ENTRY;


/* Wird fr die Druckdialoge von WDialog bentigt */
typedef struct _dither_mode
{
	struct _dither_mode  *next;	/* Zeiger auf Nachfolger     */

	long			length;				/* Strukturlnge             */
	long			format;				/* Datenformat               */
	long			reserved;			/* reserviert                */
	long			dither_id;			/* Kennung                   */
	long			color_modes;		/* untersttzte Farbtiefen   */
	long			reserved1;			/* reserviert                */
	long			reserved2;			/* reserviert                */
	char			name[32];			/* Name des Rasterverfahrens */

} DITHER_MODE;


/* Wird fr die Druckdialoge von WDialog bentigt */
typedef struct
{
	long			magic;					/* 'pdnf'                           */
	long			length;					/* Strukturlnge                    */
	long			format;					/* Datenformat                      */
	long			reserved;				/* reserviert                       */

	short			driver_id;				/* Treibernummer frs VDI           */
	short			driver_type;			/* Treibertyp                       */
	long			reserved1;				/* reserviert                       */
	long			reserved2;				/* reserviert                       */
	long			reserved3;				/* reserviert                       */

	PRN_ENTRY	*printers;				/* zum Treiber gehrenden Drucker   */
	DITHER_MODE	*dither_modes;			/* untersttzte Rasterverfahren     */
	long			reserved4;				/* reserviert                       */
	long			reserved5;				/* reserviert                       */
	long			reserved6;				/* reserviert                       */
	long			reserved7;				/* reserviert                       */
	long			reserved8;				/* reserviert                       */
	long			reserved9;				/* reserviert                       */
	char			device[128];			/* Ausgabedatei des Druckertreibers */
} DRV_INFO;

/* Flags fr die Unterdialog des Druck-Dialoges */
#define PRN_STD_SUBS	0x0001			/* Standard-Unterdialoge fr NVDI-Drucker */
#define PRN_FSM_SUBS	0x0002			/* Standard-Unterdialoge fr FSM-Drucker */
#define PRN_QD_SUBS	0x0004			/* Standard-Unterdialoge fr QuickDraw-Drucker */


/* Globale Flags des Druck-Dialoges (pdlg_create) */
#define PDLG_3D	1

/* Flags fr das ffnen des Druck-Dialoges (pdlg_open) */
#define PDLG_PREFS			0x0000	/* Einstell-Dialog            */
#define PDLG_PRINT			0x0001	/* Druck-Dialog               */

#define PDLG_ALWAYS_COPIES	0x0010	/* Immer Kopien anbieten      */
#define PDLG_ALWAYS_ORIENT	0x0020	/* Immer Querformat anbieten  */
#define PDLG_ALWAYS_SCALE	0x0040	/* Immer Skalierung anbieten  */

#define PDLG_EVENODD			0x0100	/* Option fr gerade/ungerade */
												/* Seiten anbieten            */


/* Die Buttons des Druck-Dialoges */
#define PDLG_CANCEL	1
#define PDLG_OK		2


#define PDLG_CHG_SUB		0x80000000L
#define PDLG_IS_BUTTON	0x40000000L


#define PDLG_PREBUTTON	0x20000000L
#define PDLG_PB_OK		1
#define PDLG_PB_CANCEL	2
#define PDLG_PB_DEVICE	3


#define PDLG_BUT_OK		( PDLG_PREBUTTON + PDLG_PB_OK )
#define PDLG_BUT_CNCL	( PDLG_PREBUTTON + PDLG_PB_CANCEL )
#define PDLG_BUT_DEV		( PDLG_PREBUTTON + PDLG_PB_DEVICE )



short mt_pdlg_add_printers( PRN_DIALOG *prn_dialog, DRV_INFO *drv_info, GlobalArray *globl );
short mt_pdlg_add_sub_dialogs( PRN_DIALOG *prn_dialog, PDLG_SUB *sub_dialog,
				GlobalArray *globl );
short mt_pdlg_close( PRN_DIALOG *prn_dialog, short *x, short *y, GlobalArray *globl );
PRN_DIALOG *mt_pdlg_create( const short dialog_flags, GlobalArray *globl );
short mt_pdlg_delete( PRN_DIALOG *prn_dialog, GlobalArray *globl );
short mt_pdlg_dflt_settings( PRN_DIALOG *prn_dialog, PRN_SETTINGS *settings,
				GlobalArray *globl );
short mt_pdlg_do( PRN_DIALOG *prn_dialog, PRN_SETTINGS *settings, char *document_name,
				const short option_flags, GlobalArray *globl );
short mt_pdlg_evnt( PRN_DIALOG *prn_dialog, PRN_SETTINGS *settings, EVNT *events,
				short *button, GlobalArray *globl );
short mt_pdlg_free_settings( PRN_SETTINGS *settings, GlobalArray *globl );
long mt_pdlg_get_setsize( GlobalArray *globl );
PRN_SETTINGS *mt_pdlg_new_settings( PRN_DIALOG *prn_dialog, GlobalArray *globl );
short mt_pdlg_open( PRN_DIALOG *prn_dialog, PRN_SETTINGS *settings, char *document_name,
				const short option_flags, const short x, const short y, GlobalArray *globl );
short mt_pdlg_remove_printers( PRN_DIALOG *prn_dialog, GlobalArray *globl );
short mt_pdlg_remove_sub_dialogs( PRN_DIALOG *prn_dialog, GlobalArray *globl );
short mt_pdlg_update( PRN_DIALOG *prn_dialog, char *document_name, GlobalArray *globl );
short mt_pdlg_use_settings( PRN_DIALOG *prn_dialog, PRN_SETTINGS *settings,
				GlobalArray *globl );
short mt_pdlg_validate_settings( PRN_DIALOG *prn_dialog, PRN_SETTINGS *settings,
				GlobalArray *globl );

#define pdlg_add_printers(prn_dialog, drv_info) \
													mt_pdlg_add_printers(prn_dialog, drv_info, &_globl)
#define pdlg_add_sub_dialogs(prn_dialog, sub_dialog) \
													mt_pdlg_add_sub_dialogs(prn_dialog, sub_dialog, &_globl)
#define pdlg_close(prn_dialog, x, y)	mt_pdlg_close(prn_dialog, x, y, &_globl)
#define pdlg_create(dialog_flags)		mt_pdlg_create(dialog_flags, &_globl)
#define pdlg_delete(prn_dialog)			mt_pdlg_delete(prn_dialog, &_globl)
#define pdlg_dflt_settings(prn_dialog, settings) \
													mt_pdlg_dflt_settings(prn_dialog, settings, &_globl)
#define pdlg_do(prn_dialog, settings, document_name, option_flags) \
													mt_pdlg_do(prn_dialog, settings, document_name, option_flags, &_globl)
#define pdlg_evnt(prn_dialog, settings, events, button) \
													mt_pdlg_evnt(prn_dialog, settings, events, button, &_globl)
#define pdlg_free_settings(settings)	mt_pdlg_free_settings(settings, &_globl)
#define pdlg_get_setsize()					mt_pdlg_get_setsize(&_globl)
#define pdlg_new_settings(prn_dialog)	mt_pdlg_new_settings(prn_dialog, &_globl)
#define pdlg_open(prn_dialog, settings, document_name, option_flags, x, y) \
													mt_pdlg_open(prn_dialog, settings, document_name, option_flags, x, y, &_globl)
#define pdlg_remove_printers(prn_dialog) \
													mt_pdlg_remove_printers(prn_dialog, &_globl)
#define pdlg_remove_sub_dialogs(prn_dialog) \
													mt_pdlg_remove_sub_dialogs(prn_dialog, &_globl)
#define pdlg_update(prn_dialog, document_name) \
													mt_pdlg_update(prn_dialog, document_name, &_globl)
#define pdlg_use_settings(prn_dialog, settings) \
													mt_pdlg_use_settings(prn_dialog, settings, &_globl)
#define pdlg_validate_settings(prn_dialog, settings) \
													mt_pdlg_validate_settings(prn_dialog, settings, &_globl)


/*-------------------------------------------------------------------
	Die WDLG-Funktionen (MagiC, WDialog)
 -------------------------------------------------------------------*/

/* Fr die WDLG-Funktionen */
typedef short cdecl (*HNDL_OBJ)(DIALOG *dialog, EVNT *events, short obj,
										short clicks, void *data);

/* Definitionen fr <flags> fr wdlg_create */
#define  WDLG_BKGD   1           /* Permit background operation */

/* Funktionsnummern fr <obj> in handle_exit(...) bei wdlg_create */
#define  HNDL_INIT   -1          /* Initialise dialog */
#define  HNDL_MESG   -2          /* Initialise dialog */
#define  HNDL_CLSD   -3          /* Dialog window was closed */
#define  HNDL_OPEN   -5          /* End of dialog initialisation (second  call at end of wdlg_init) */
#define  HNDL_EDIT   -6          /* Test characters for an edit-field */
#define  HNDL_EDDN   -7          /* Character was entered in edit-field */
#define  HNDL_EDCH   -8          /* Edit-field was changed */
#define  HNDL_MOVE   -9          /* Dialog was moved */
#define  HNDL_TOPW   -10         /* Dialog-window has been topped */
#define  HNDL_UNTP   -11         /* Dialog-window is not active */


DIALOG *mt_wdlg_create( HNDL_OBJ handle_exit, OBJECT *tree, void *user_data, const short code,
				void *data, const short flags, GlobalArray *globl );
short	mt_wdlg_open( DIALOG *dialog, char *title, const short kind, const short x,
				const short y, const short code, void *data, GlobalArray *globl );
short mt_wdlg_close( DIALOG *dialog, short *x, short *y, GlobalArray *globl );
short mt_wdlg_delete( DIALOG *dialog, GlobalArray *globl );
short mt_wdlg_get_tree( DIALOG *dialog, OBJECT **tree, GRECT *r, GlobalArray *globl );
short mt_wdlg_get_edit( DIALOG *dialog, short *cursor, GlobalArray *globl );
void *mt_wdlg_get_udata( DIALOG *dialog, GlobalArray *globl );
short mt_wdlg_get_handle( DIALOG *dialog, GlobalArray *globl );
short mt_wdlg_set_edit( DIALOG *dialog, const short obj, GlobalArray *globl );
short mt_wdlg_set_tree( DIALOG *dialog, OBJECT *new_tree, GlobalArray *globl );
short mt_wdlg_set_size( DIALOG *dialog, GRECT *new_size, GlobalArray *globl );
short mt_wdlg_set_iconify( DIALOG *dialog, GRECT *g, char *title, OBJECT *tree,
				const short obj, GlobalArray *globl );
short mt_wdlg_set_uniconify( DIALOG *dialog, GRECT *g, char *title, OBJECT *tree,
				GlobalArray *globl );
short mt_wdlg_evnt( DIALOG *dialog, EVNT *events, GlobalArray *globl );
void mt_wdlg_redraw( DIALOG *dialog, GRECT *rect, const short obj, const short depth,
				GlobalArray *globl );

#define wdlg_create(handle_exit, tree, user_data, code, data, flags) \
														mt_wdlg_create(handle_exit, tree, user_data, code, data, flags, &_globl)
#define wdlg_open(dialog, title, kind, x, y, code, data) \
														mt_wdlg_open(dialog, title, kind, x, y, code, data, &_globl)
#define wdlg_close(dialog, x, y)				mt_wdlg_close(dialog, x, y, &_globl)
#define wdlg_delete(dialog)					mt_wdlg_delete(dialog, &_globl)
#define wdlg_get_tree(dialog, tree, r)		mt_wdlg_get_tree(dialog, tree, r, &_globl)
#define wdlg_get_edit(dialog, cursor)		mt_wdlg_get_edit(dialog, cursor, &_globl)
#define wdlg_get_udata(dialog)				mt_wdlg_get_udata(dialog, &_globl)
#define wdlg_get_handle(dialog)				mt_wdlg_get_handle(dialog, &_globl)
#define wdlg_set_edit(dialog, obj)			mt_wdlg_set_edit(dialog, obj, &_globl)
#define wdlg_set_tree(dialog, new_tree)	mt_wdlg_set_tree(dialog, new_tree, &_globl)
#define wdlg_set_size(dialog, new_size)	mt_wdlg_set_size(dialog, new_size, &_globl)
#define wdlg_set_iconify(dialog, g, title, tree, obj) \
														mt_wdlg_set_iconify(dialog, g, title, tree, obj, &_globl)
#define wdlg_set_uniconify(dialog, g, title, tree) \
														mt_wdlg_set_uniconify(dialog, g, title, tree, &_globl)
#define wdlg_evnt(dialog, events)			mt_wdlg_evnt(dialog, events, &_globl)
#define wdlg_redraw(dialog, rect, obj, depth) \
														mt_wdlg_redraw(dialog, rect, obj, depth, &_globl)


/*-------------------------------------------------------------------
	Die EDIT-Funktionen (MagiC, WDialog)
 -------------------------------------------------------------------*/

typedef void *XEDITINFO;


XEDITINFO *mt_edit_create( GlobalArray *globl );
void mt_edit_set_buf( OBJECT *tree, const short obj, char *buffer, const long buflen,
				GlobalArray *globl );
short mt_edit_open( OBJECT *tree, const short obj, GlobalArray *globl );
void mt_edit_close( OBJECT *tree, const short obj, GlobalArray *globl );
void mt_edit_delete( XEDITINFO *editinfo, GlobalArray *globl );
short mt_edit_cursor( OBJECT *tree, const short obj, const short whdl, const short show,
				GlobalArray *globl );
short mt_edit_evnt( OBJECT *tree, const short obj, const short whdl, EVNT *events,
				long *errcode, GlobalArray *globl );
short mt_edit_get_buf( OBJECT *tree, const short obj, char **buffer, long *buflen,
				long *txtlen, GlobalArray *globl );
short mt_edit_get_format( OBJECT *tree, const short obj, short *tabwidth, short *autowrap,
				GlobalArray *globl );
short mt_edit_get_colour( OBJECT *tree, const short obj, short *tcolour, short *bcolour,
				GlobalArray *globl );
short mt_edit_get_font( OBJECT *tree, const short obj, short *fontID, short *fontH,
				boolean *fontPix, boolean *mono, GlobalArray *globl );
short mt_edit_get_cursor( OBJECT *tree, const short obj, char **cursorpos,
				GlobalArray *globl );
void mt_edit_get_pos( OBJECT *tree, const short obj, short *xscroll, long *yscroll,
				char **cyscroll, char **cursorpos, short *cx, short *cy, GlobalArray *globl );
boolean mt_edit_get_dirty( OBJECT *tree, const short obj, GlobalArray *globl );
void mt_edit_get_sel( OBJECT *tree, const short obj, char **bsel, char **esel,
				GlobalArray *globl );
void mt_edit_get_scrollinfo( OBJECT *tree, const short obj, long *nlines, long *yscroll,
				short *yvis, short *yval, short *ncols, short *xscroll, short *xvis,
				GlobalArray *globl );
void mt_edit_set_format( OBJECT *tree, const short obj, const short tabwidth,
				const short autowrap, GlobalArray *globl );
void mt_edit_set_colour( OBJECT *tree, const short obj, const short tcolour,
				const short bcolour, GlobalArray *globl );
void mt_edit_set_font( OBJECT *tree, const short obj, const short fontID, const short fontH,
				const boolean fontPix, const boolean mono, GlobalArray *globl );
void mt_edit_set_cursor( OBJECT *tree, const short obj, char *cursorpos, GlobalArray *globl );
void mt_edit_set_pos( OBJECT *tree, const short obj, const short xscroll,
				const long yscroll, char *cyscroll, char *cursorpos, const short cx,
				const short cy, GlobalArray *globl );
short mt_edit_resized( OBJECT *tree, const short obj, short *oldrh, short *newrh,
				GlobalArray *globl );
void mt_edit_set_dirty( OBJECT *tree, const short obj, const boolean dirty,
				GlobalArray *globl );
short mt_edit_scroll( OBJECT *tree, const short obj, const short whdl, const long yscroll,
				const short xscroll, GlobalArray *globl );

/* edit_get_colour = edit_get_color? Ein Schreibfeher in der MagiC-Dokumentation? */
short mt_edit_get_color( OBJECT *tree, const short obj, short *tcolor, short *bcolor,
				GlobalArray *globl );

/* edit_set_colour = edit_set_color? Ein Schreibfeher in der MagiC-Dokumentation? */
void mt_edit_set_color( OBJECT *tree, const short obj, const short tcolor, const short bcolor,
				GlobalArray *globl );

#define edit_create()								mt_edit_create(&_globl)
#define edit_set_buf(tree, obj, buffer, buflen) \
															mt_edit_set_buf(tree, obj, buffer, buflen, &_globl)
#define edit_open(tree, obj)						mt_edit_open(tree, obj, &_globl)
#define edit_close(tree, obj)						mt_edit_close(tree, obj, &_globl)
#define edit_delete(editinfo)						mt_edit_delete(editinfo, &_globl)
#define edit_cursor(tree, obj, whdl, show)	mt_edit_cursor(tree, obj, whdl, show, &_globl)
#define edit_evnt(tree, obj, whdl, events, errcode) \
															mt_edit_evnt(tree, obj, whdl, events, errcode, &_globl)
#define edit_get_buf(tree, obj, buffer, buflen, txtlen) \
															mt_edit_get_buf(tree, obj, buffer, buflen, txtlen, &_globl)
#define edit_get_format(tree, obj, tabwidth, autowrap) \
															mt_edit_get_format(tree, obj, tabwidth, autowrap, &_globl)
#define edit_get_colour(tree, obj, tcolour, bcolour) \
															mt_edit_get_colour(tree, obj, tcolour, bcolour, &_globl)
#define edit_get_font(tree, obj, fontID, fontH, fontPix, mono) \
															mt_edit_get_font(tree, obj, fontID, fontH, fontPix, mono, &_globl)
#define edit_get_cursor(tree, obj, cursorpos) \
															mt_edit_get_cursor(tree, obj, cursorpos, &_globl)
#define edit_get_pos(tree, obj, xscroll, yscroll, cyscroll, cursorpos, cx, cy) \
															mt_edit_get_pos(tree, obj, xscroll, yscroll, cyscroll, cursorpos, cx, cy, &_globl)
#define edit_get_dirty(tree, obj)				mt_edit_get_dirty(tree, obj, &_globl)
#define edit_get_sel(tree, obj, bsel, esel)	mt_edit_get_sel(tree, obj, bsel, esel, &_globl)
#define edit_get_scrollinfo(tree, obj, nlines, yscroll, yvis, yval, ncols, xscroll, xvis) \
															mt_edit_get_scrollinfo(tree, obj, nlines, yscroll, yvis, yval, ncols, xscroll, xvis, &_globl)
#define edit_set_format(tree, obj, tabwidth, autowrap) \
															mt_edit_set_format(tree, obj, tabwidth, autowrap, &_globl)
#define edit_set_colour(tree, obj, tcolour, bcolour) \
															mt_edit_set_colour(tree, obj, tcolour, bcolour, &_globl)
#define edit_set_font(tree, obj, fontID, fontH, fontPix, mono) \
															mt_edit_set_font(tree, obj, fontID, fontH, fontPix, mono, &_globl)
#define edit_set_cursor(tree, obj, cursorpos) \
															mt_edit_set_cursor(tree, obj, cursorpos, &_globl)
#define edit_set_pos(tree, obj, xscroll, yscroll, cyscroll, cursorpos, cx, cy) \
															mt_edit_set_pos(tree, obj, xscroll, yscroll, cyscroll, cursorpos, cx, cy, &_globl)
#define edit_resized(tree, obj, oldrh, newrh) \
															mt_edit_resized(tree, obj, oldrh, newrh, &_globl)
#define edit_set_dirty(tree, obj, dirty)		mt_edit_set_dirty(tree, obj, dirty, &_globl)
#define edit_scroll(tree, obj, whdl, yscroll, xscroll) \
															mt_edit_scroll(tree, obj, whdl, yscroll, xscroll, &_globl)

/* edit_get_colour = edit_get_color? Ein Schreibfeher in der MagiC-Dokumentation? */
#define edit_get_color(tree, obj, tcolor, bcolor) \
															mt_edit_get_colour(tree, obj, tcolor, bcolor, &_globl)

/* edit_set_colour = edit_set_color? Ein Schreibfeher in der MagiC-Dokumentation? */
#define edit_set_color(tree, obj, tcolor, bcolor) \
															mt_edit_set_colour(tree, obj, tcolor, bcolor, &_globl)


/*------------------------------------------------------------------*/

#endif
