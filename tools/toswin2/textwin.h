
#ifndef tw_textwin_h
# define tw_textwin_h 1

#ifndef tw_window_h
# include "window.h"
#endif
#ifndef tw_config_h
# include "config.h"
#endif

/* text->cflag */
#define CBGCOL		 0x000f	/* background color mask */
#define CFGCOL		 0x00f0	/* foreground color mask */
#define COLORS(fg, bg)	 (((fg) << 4) | (bg))
#define CEFFECTS	 0x0f00	/* VDI output style mask */
#define CE_ANSI_EFFECTS (CE_BOLD | CE_LIGHT)
#define CE_BOLD		 0x0100
#define CE_LIGHT	 0x0200
#define CE_ITALIC	 0x0400
#define CE_UNDERLINE	 0x0800
#define CINVERSE	 0x1000	/* character is in inverse video */

/* Line drawing is activated for G1 designator with argument
   0.  It can therefore be restored from the terminal flags.
   We only save the result with the character.  */
#define CLINEDRAW	 0x2000

#define CPROTECTED	 0x4000 /* Character drawn that cannot be erased.  */
#define CINVISIBLE	 0x8000 /* Invisible character.  */

#define CNONSPACE	0x10000 /* For future use: non-zero if a character has
				   been drawn here.  */
#define CSELECTED	0x20000 /* Character has been selected by mouse.  */
#define CDIRTY		0x40000 /* Character has changed since last redraw.  */

#define ATTRIBUTES (CBGCOL | CFGCOL | \
		    CE_BOLD | CE_LIGHT | CE_ITALIC | CE_UNDERLINE | \
		    CINVERSE | CINVISIBLE | CPROTECTED)
#define DECSC_FLAGS (ATTRIBUTES | TORIGIN | TWRAPAROUND)

/* Per terminal flags.  */
#define TCSG		   0x1 /* G1 if non-zero, G0 else.  */
#define TCSGS		   0x2 /* '0' if non-zero, 'B' else.  */
#define TINSERT		   0x4 /* Non-zero if in insert mode.  */
#define TORIGIN		   0x8 /* Non-zero if in origin (relative addressing 
			          mode).  */
#define TWRAPAROUND	  0x10 /* Non-zero if in auto wraparound mode.  */
#define TDECCOLM	  0x20 /* Non-zero if 80/132 column switch enabled.  */
#define TCURS_ON	  0x40 /* Non-zero if cursor visible.  */
#define TCURS_VVIS	  0x80 /* Non-zero if cursor very visible.  */
#define TDECSCNM	 0x100 /* Non-zero if in reverse-video mode.  */

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

/* Tab positions */
typedef struct tablist TABLIST;
struct tablist
{
	short	tabpos;		/* character position of tab */
  	TABLIST	*nexttab;	/* pointer to next tab position */
};

typedef struct textwin TEXTWIN;
struct textwin
{
	WINDOW* win;				/* underlying WINDOW struct */
	int	offy;				/* offset of window (0, 0) position */
	short	maxx;				/* number of characters across */
	short	miny;				/* first 'real' line (previous lines are scrollback */
	short	maxy;				/* number of characters down */
	short	cx, cy;				/* current cursor position (character coord.) */
	short	cmaxwidth;			/* max. character width */
	short	cheight;			/* character height */
	short	cbase;				/* distance from character baseline to top */
	short	cfont;				/* font for characters */
	short	cpoints;			/* size of characters in points */
	short	saved_x, saved_y;		/* Saved cursor position.  */
	unsigned long saved_cattr;		/* Saved attributes.  */
	unsigned short saved_tflags;		/* Saved terminal flags.  */
	unsigned short curr_tflags;		/* e.g. cursor on/off */
	unsigned long curr_cattr;		/* current character attributes including
						   foreground/background colors and character
						   set */
	unsigned short escy1;			/* first char. for ESC Y */
	void	(*output)(struct textwin *t,
			   unsigned int c); 	/* output function */
	unsigned short alloc_width;		/* Max. width that the following buffers can
						   handle.  */
	unsigned short alloc_height;		/* Max. height that the following buffers can
						   handle.  */
	unsigned char **cdata;			/* terminal data */
	unsigned long **cflags;			/* flags for individual characters */
	
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

	unsigned char escbuf[ESCBUFSIZE];	/* Buffer for multiple char escapes */
	short	 vt_mode;			/* Terminal emulator mode */
	short	 scroll_top;	  		/* Top line for scrolling region */
	short	 scroll_bottom;  		/* Bottom line for scrolling region */
	TABLIST *tabs;			  	/* List of tab positions */
	short 	curs_mode;			/* Mode for cursor keys */

	short	talkID;				/* ID vom aufrufenden TalkTW */
	int	shell;				/* Typ der Shell */

	WINCFG	*cfg;				/* Zeiger auf die Konfig (NULL = Default) */
	int	block_x1,			/* enth„lt Block-Daten (im Zeichenmaž!) */
		block_x2,
		block_y1,
		block_y2;
	unsigned long fg_effects;		/* Bit vector of text effects.  */
	unsigned long bg_effects;		/* Bit vector with background effects
						   (only CE_BOLD/CE_LIGHT are used).  */

	int	curs_height;			/* Height of the cursor block.  */
	int	curs_offy;			/* Distance from bottom of cursor to bottom
							 of line.  */
	short	last_cx, last_cy;		/* Last position of cursor.  */

	/* Various flags.  */
	unsigned wintop: 1;			/* 1 - if active window.  */
	unsigned curs_drawn: 1;			/* 1 if cursor currently showing.  */
	unsigned vdi_colors: 1;			/* Non-zero if vdi colors active.  */
	unsigned windirty: 1;			/* Non-zero if window size has changed.  */
	unsigned do_wrap: 1;			/* Non-zero if cursor in last column and
							 character just output.  */
};

/* Character encoding */
#define TAB_ATARI		0x0000	/* normal Atari */
#define TAB_ISO			0x0001	/* ISO 8859-1 Latin1 */

/* Gr”žen des Textfensters berechnen */
#define SCROLLBACK(t)	((t)->miny)
#define NROWS(t) 	((t)->maxy - (t)->miny)
#define NCOLS(t) 	((t)->maxx)

/* Relative offset for origin mode.  */
#define RELOFFSET(t)	(((t)->curr_tflags & TORIGIN) ? \
	((t)->scroll_top - (t)->miny) : 0)

void	char2pixel (TEXTWIN *t, short col, short row, short *xp, short *yp);
void 	pixel2char (TEXTWIN *t, short x, short y, short *colp, short *rowp);
void 	mark_clean (TEXTWIN *t);
void 	refresh_textwin (TEXTWIN *t, short force);

TEXTWIN *create_textwin (char *title, WINCFG *cfg);
void 	destroy_textwin (TEXTWIN *t);

void 	resize_textwin (TEXTWIN *t, short cols, short rows, short scrollback);
void	textwin_setfont (TEXTWIN *t, short font, short points);
void	reconfig_textwin (TEXTWIN *t, WINCFG *cfg);

void 	sendstr (TEXTWIN *t, char *s);
void	write_text (TEXTWIN *t, char *b, long len);

TEXTWIN *get_textwin (short handle, short talkID);

void	textwin_term (void);


#endif
