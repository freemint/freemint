
#ifndef _tw_global_h_
#define _tw_global_h_

#include <sys/types.h>
typedef unsigned char uchar;

#include <cflib.h>

#if !defined(__CFLIB__)
#error at least cflib 0.20.0 is required to compile TosWin2
#endif


typedef enum {FALSE, TRUE} bool;

/*
 * Typen
 */

/* win->flags */
#define WFULLED		1
#define WICONIFIED 	2
#define WSHADED 	4
#define WISDIAL		8


typedef struct win WINDOW;
struct win
{
	WINDOW	*next;		/* next window in list */
	int	handle;		/* AES window handle */
	int	kind;		/* window gadgets */
	char 	*title;
	GRECT	work;		/* size of window working area */
	GRECT	full;
	GRECT	prev;
	int     max_w, max_h;	/* max size of window working area */
	int	icon_x, icon_y;	/* WINICON Position, da der Objektbaum mehrfach benutzt wird. */
	int	flags;		/* various window flags */
	int	old_wkind;	/* old window gadgets before iconification */
	void	*extra;		/* Pointer to extra data for subclasses */

	void	(*draw) 	(struct win *win, short x, short y, short w, short h);
	void	(*topped) 	(struct win *win);
  	void    (*ontopped) 	(struct win *win);
	void    (*untopped) 	(struct win *win);
	void    (*bottomed) 	(struct win *win);
	void	(*closed) 	(struct win *win);
	void	(*fulled) 	(struct win *win);
	void	(*sized) 	(struct win *win, short x, short y, short w, short h);
	void	(*moved) 	(struct win *win, short x, short y, short w, short h);
	void	(*iconify) 	(struct win *win, short, short, short, short);
	void	(*uniconify)	(struct win *win, short, short, short, short);
	void	(*shaded) 	(struct win *win, short on);
	void	(*arrowed) 	(struct win *win, short msg);
	void	(*hslid) 	(struct win *win, short hpos);
	void	(*vslid) 	(struct win *win, short vpos);
	bool	(*keyinp) 	(struct win *win, short keycode, short shft );
	bool	(*mouseinp)	(struct win *win, short clicks, short x, short y, short shft, short mbuttons);
	void	(*oldfulled)	(struct win *win);
	bool	(*oldmouse)	(struct win *win, short clicks, short x, short y, short shft, short mbuttons);
};


/* Character encoding */
#define TAB_ATARI		0x0000	/* normal Atari */
#define TAB_ISO			0x0001	/* ISO 8859-1 Latin1 */

typedef struct _wincfg WINCFG;
struct _wincfg
{
	WINCFG	*next;
	char	progname[80];
	char	arg[50];
	char	title[80];
	int	kind;
	int	font_id, font_pts;
	int	col, row, scroll;
	int	xpos, ypos;
	int	width, height;		/* 'offene' Gr”že */
	bool	iconified;
	bool	wrap;
	bool	autoclose;
	int	vt_mode;
	int	fg_color;
	int	bg_color;
	int	char_tab;
};


/* Tab positions */
typedef struct tablist TABLIST;
struct tablist 
{
	short	tabpos;		/* character position of tab */
  	TABLIST	*nexttab;	/* pointer to next tab position */
};


/* Gr”žen des Textfensters berechnen */
#define SCROLLBACK(t)	((t)->miny)
#define NROWS(t) 	((t)->maxy - (t)->miny)
#define NCOLS(t) 	((t)->maxx)

/* Min/Maximale Fenstergr”že */
#define MINROWS	1
#define MINCOLS	10
#define MAXROWS	128
#define MAXCOLS	220

/* text->term_flags */
#define FINSERT	0x1000		/* insert characters */
#define FFLASH		0x2000	/* cursor is currently showing */
#define FCURS		0x4000	/* cursor enabled */
#define FWRAP		0x8000	/* wrap at end of line */

/* text->cflag */
#define CBGCOL		0x000f	/* background color mask */
#define CFGCOL		0x00f0	/* foreground color mask */
#define COLORS(fg, bg)	(((fg) << 4) | (bg))
#define CEFFECTS	0x0f00	/* VDI output style mask */
#define CE_BOLD		0x0100
#define CE_LIGHT	0x0200
#define CE_ITALIC	0x0400
#define CE_UNDERLINE	0x0800
#define CINVERSE	0x1000	/* character is in inverse video */
#define CSELECTED	0x2000	/* character has been selected by mouse */
#define CTOUCHED	0x4000	/* character attributes have changed */
#define CDIRTY		0x8000	/* the character itself has changed */

/* text->dirty */
#define SOMEDIRTY	0x01	/* some of the line needs redrawing */
#define ALLDIRTY	0x02	/* whole line is dirty */

/* text ->cursmode */
#define CURSOR_NORMAL 	1
#define CURSOR_TRANSMIT 2

/* text->vt_mode */
#define MODE_VT52 	1
#define MODE_VT100 	2

/* text->char_set */
#define NORM_CHAR	0	/* ESC (B : normal ASCII */
#define LINE_CHAR	1	/* ESC (0 : Line drawing */

#define CAPTURESIZE 256
#define ESCBUFSIZE 128

/* text->shell */
#define NO_SHELL	0
#define NORM_SHELL	1
#define LOGIN_SHELL	2

typedef struct textwin TEXTWIN;
struct textwin
{
	WINDOW	*win;				/* underlying WINDOW struct */
	int	offx, offy;			/* offset of window (0, 0) position */
	short	maxx;				/* number of characters across */
	short	miny;				/* first 'real' line (previous lines are scrollback */
	short	maxy;				/* number of characters down */
	short	cx, cy;				/* current cursor position (character coord.) */
	short	cmaxwidth;			/* max. character width */
	short	cheight;			/* character height */
	short	cbase;				/* distance from character baseline to top */
	short	cfont;				/* font for characters */
	short	cpoints;			/* size of characters in points */
	short	savex, savey;			/* saved cursor position */
	short	term_flags;			/* e.g. cursor on/off */
	short	term_cattr;			/* current character attributes */
										/* also foreground and background colors */
	short	escy1;				/* first char. for ESC Y */
	void	(*output)(struct textwin *t, int c); /* output function */
	uchar	**data;				/* terminal data */
	short	**cflag;			/* flags for individual characters */
	char	*dirty;				/* marks whether lines need redrawing */
	int	fd;				/* file descriptor for pseudo-tty */
	char	pty[8];				/* Pseudo-TTY auf U:\pipe\ttyp[0-f] */
	int	pgrp;				/* process group for terminal */
	long	scrolled;			/* number of lines to scroll before re-draw */
	long	nbytes;				/* number of bytes written to window since last refresh */
	long	draw_time;			/* set to indicate how long it's been since the last refresh */
	char	*prog;				/* program name */
	char	*cmdlin;			/* program command line */
	char	*progdir;			/* program current directory */
	void	(*callback)(struct textwin *t);	/* call this when the capture buffer is full */
	char	captbuf[CAPTURESIZE+1];
	int	captsiz;
	int	minADE, maxADE;			/* min. and max. character the font can display */
	short	*cwidths;			/* table of font widths */

	char	escbuf[ESCBUFSIZE];		/* Buffer for multiple char escapes */
	short   vt_mode;			/* Terminal emulator mode */
	short	char_set;			/* Zeichensatz */
	short   scroll_top;	  		/* Top line for scrolling region */
	short   scroll_bottom;  		/* Bottom line for scrolling region */
	TABLIST *tabs;			  	/* List of tab positions */
	short 	curs_mode;			/* Mode for cursor keys */

	short	talkID;				/* ID vom aufrufenden TalkTW */
	int	shell;				/* Typ der Shell */

	WINCFG	*cfg;				/* Zeiger auf die Konfig (NULL = Default) */
	int	block_x1,			/* enth„lt Block-Daten (im Zeichenmaž!) */
		block_x2,
		block_y1,
		block_y2;
};


#define TW2NAME	"TOSWIN2 "

/* Dateinamen */
#define CFGNAME		"toswin2.cfg"
#define RSCNAME		"toswin2.rsc"
#define XCONNAME	"u:\\dev\\xconout2"
#define TWCONNAME	"u:\\dev\\tw-con"

/*
 * Globale Variablen
*/
extern OBJECT	*winicon,
		*conicon,
		*strings;

extern int	exit_code;		/* Enth„lt den Exitcode eines Kindprozesses */
extern int	vdi_handle;
extern int	font_anz;

/*
 * Translation tables
*/
extern unsigned char st_to_iso[];
extern unsigned char iso_to_st[];


/*
 * Hilfsfunktionen
 */

#define	rsc_string(a)	strings[a].ob_spec.free_string

void	set_fillcolor(int col);
void	set_textcolor(int col);
void	set_texteffects(int effects);
void	set_wrmode(int mode);
void	set_font(int font, int height);
void 	set_fillstyle(int style, int /* index */);

void	draw_winicon(WINDOW *win);

int	alert(int def, int undo, int num);

void	global_init(void);
void	global_term(void);


#endif
