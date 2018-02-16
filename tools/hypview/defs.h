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

#ifndef	DEFS_H
#define	DEFS_H		1
#define	VERSION "0.40.0"

#define	LINE_BUF		512

/* Return values for file loading routines  */
enum {
	F_LOADERROR = -2,       /* Error while loading */
	F_UNKNOWN = -1,         /* Unknown file type */
	F_BINARY = 0,           /* binary format */
	F_ASCII,                /* ASCII format */
	F_HYP,                  /* ST-Guide hypertext format */
};

typedef struct {
	long line;              /* Line number */
	long y;
	long offset;            /* Char. offset to beginning of word */
	short x;                /* x coordinate of lower left corner */
}TEXT_POS;

typedef struct
{
	short valid;            /* Flag indicating validity of content */
	TEXT_POS start;         /* Start of block */
	TEXT_POS end;           /* End of block */
}BLOCK;

#define	BLK_CHKOP           0       /* Check if operation is supported */
#define	BLK_ASCIISAVE       1       /* Save current block as ASCII */
#define	BLK_PRINT           2       /* Print current block */

/* Anonymous document type specific functions */
struct _document_;
typedef	void (*DOC_PROC1)(struct _document_ *doc);
typedef	long (*DOC_GETNODEPROC)(struct _document_ *doc);
typedef	void (*DOC_GOTOPROC)(struct _document_ *doc, char *chapter, long node);
typedef	void (*DOC_CLICKPROC)(struct _document_ *doc, EVNTDATA *event);
typedef	long (*DOC_AUTOLOCPROC)(struct _document_ *doc, long line);
typedef	void (*DOC_GETCURSORPROC)(struct _document_ *doc, short x, short y, TEXT_POS *pos);
typedef	short (*DOC_BLOCKPROC)(struct _document_ *doc, short op, BLOCK *block, void *param);

struct _document_
{
	struct _document_ *next;    /* Pointer to next document */
	short type;                 /* Document type see F_xy constants */
	char *path;                 /* Full file access path */
	char *filename;             /* File name */
	char *window_title;         /* Window titel */
	long start_line;            /* First visible line of document */
	long lines;                 /* Number of lines (in window lines) */
	long height;
	long columns;               /* Number of window columns */
	short buttons;              /* Toolbar button configuration (bit vector)*/
	void *data;                 /* File format specific data */
	short mtime,mdate;          /* File modification time and date */
	WINDOW_DATA *window;        /* Window associated with this file */
	WINDOW_DATA *popup;			/* Currently activ popup window */
	DOC_PROC1 displayProc;      /* Document display function */
	DOC_PROC1 closeProc;        /* Document close function */
	DOC_GOTOPROC gotoNodeProc;  /* Document navigation function */
	DOC_GETNODEPROC getNodeProc;/* Function to determine current node number */
	DOC_CLICKPROC clickProc;    /* Mouse-click processing function */
	DOC_AUTOLOCPROC autolocProc;/* Autolocator search function */
	char *autolocator;          /* Autolocator search string */
	int autolocator_dir;        /* Autolocator direction (1 = down, else up) */
	DOC_GETCURSORPROC getCursorProc;/* Cursor position function */
	BLOCK selection;            /* Content of  selection */
	DOC_BLOCKPROC blockProc;    /* Block operation function */
};
typedef struct _document_ DOCUMENT;


/*	Convert toolbar object number into bit values for DOCUMENT->buttons */
#define BITVAL(x)	(1<<(x-4))


/*
 *		Global.c
 */
extern char path_list[];
extern char default_file[];
extern char catalog_file[];
extern char file_extensions[];
extern char dir_separator;
extern short binary_columns;
extern short ascii_tab_size;
extern short ascii_break_len;
extern WINDOW_DATA *Win;
extern short win_x,win_y,win_w,win_h;
extern short adjust_winsize;
extern short intelligent_fuller;
extern short sel_font_id,sel_font_pt;
extern short font_id,font_pt;
extern short xfont_id,xfont_pt;
extern short font_h, font_w, font_cw, font_ch;
extern short background_col;
extern short text_col,link_col,link_effect;
extern short transparent_pics;
extern char *av_parameter;
extern short va_start_newwin;
extern char debug_file[];
extern short check_time;
extern char skin_path[];
extern GlobalArray skin_global;
extern short clipbrd_new_window;
extern short av_window_cycle;
extern char marken_path[];
extern short marken_save_ask;
extern char all_ref[];
extern char hypfind_path[];
extern short refonly;

/*
 *		Config.c
 */
void LoadConfig(void);

/*
 *		Init.c
 */
void Init(void);
void Exit(void);

/*
 *		Fileselc.c
 */
void SelectFileLoad(void);
void SelectFileSave(DOCUMENT *doc);

/*
 *		File.c
 */
short LoadFile(DOCUMENT *doc, short handle);
void CloseFile(DOCUMENT *doc);
short OpenFileNW(char *path, char *chapter, long node);
short OpenFileSW(char *path, char *chapter,short new_win);
void CheckFiledate(DOCUMENT *doc);

/*
 *		Error.c
 */
void FileError(char *path,char *str);
void FileErrorNr(char *path,long ret);



/*
 *		Ascii.c
 */
short AsciiLoad(DOCUMENT *doc, short handle);

/*
 *		hyp\Hyp.c
 */
short HypLoad(DOCUMENT *doc, short handle);

/*
 *		Window.c
 */
void SendClose(WINDOW_DATA *wind);
void SendRedraw(WINDOW_DATA *wind);
void ReInitWindow(WINDOW_DATA *wind, DOCUMENT *doc);
short HelpWindow(WINDOW_DATA *ptr, short obj, void *data);

/*
 *		Keyrepet.c
 */
void KeyboardOnOff(void);

/*
 *		Toolbar.c
 */
void ToolbarUpdate(DOCUMENT *doc, OBJECT *toolbar, short redraw);
void ToolbarClick(DOCUMENT *doc, short obj);
void RemoveSearchBox(DOCUMENT *doc);

/*
 *		History.c
 */
typedef struct _history_
{
	struct _history_ *next;      /* Pointer to next history entry */
	WINDOW_DATA *win;            /* Associated window */
	DOCUMENT *doc;               /* Pointer to document */
	long line;                   /* First visible line */
	long node;                   /* Document node (=chapter) number */
	char title;                  /* First byte of history title */
}HISTORY;

extern HISTORY *history;         /* Pointer to history data */

void AddHistoryEntry(WINDOW_DATA *wind);
short RemoveHistoryEntry(DOCUMENT **doc,long *node,long *line);
void RemoveAllHistoryEntries(WINDOW_DATA *wind);
short CountWindowHistoryEntries(WINDOW_DATA *wind);
short CountDocumentHistoryEntries(DOCUMENT *doc);
void DeletLastHistory(void *entry);
void *GetLastHistory(void);
void SetLastHistory(WINDOW_DATA *the_win,void *last);

/*
 *		Tools.c
 */
WINDOW_DATA *get_first_window(void);

/*
 *		Navigate.c
 */
void GotoPage(DOCUMENT *doc, long num, long line, short calc);
void GoBack(DOCUMENT *doc);
void MoreBackPopup(DOCUMENT *doc, short x, short y);
void GotoHelp(DOCUMENT *doc);
void GotoIndex(DOCUMENT *doc);
void GoThisButton(DOCUMENT *doc,short obj);

/*
 *		Autoloc.c
 */
short AutolocatorKey(DOCUMENT *doc, short kbstate, short ascii);
void AutoLocatorPaste(DOCUMENT *doc);

/*
 *		selectio.c
 */
void SelectAll(DOCUMENT *doc);
void MouseSelection(DOCUMENT *doc,EVNTDATA *m_data);
void RemoveSelection(DOCUMENT *doc);
void DrawSelection(DOCUMENT *doc);

/*	
 *		Font.c
 */
short ProportionalFont(short *width);
void SwitchFont(DOCUMENT *doc);

/*
 *		Block.c
 */
void BlockOperation(DOCUMENT *doc, short num);
void BlockSelectAll(DOCUMENT *doc, BLOCK *b);
void BlockCopy(DOCUMENT *doc);
void BlockPaste(short new_window);
void BlockAsciiSave(DOCUMENT *doc, char *path);

/*
 *		Marker.c
 */
void MarkerSave(DOCUMENT *doc, short num);
void MarkerShow(DOCUMENT *doc, short num, short new_window);
void MarkerPopup(DOCUMENT *doc, short x, short y);
void MarkerSaveToDisk(void);
void MarkerInit(void);

/*
 *		Info.c
 */
void ProgrammInfos(DOCUMENT *doc);

/*
 *		search.c
 */
void search_all(char *string);
void Search(DOCUMENT *doc);

/*
 *		hyp\search.c
 */
void Hypfind( DOCUMENT *doc );

#endif
