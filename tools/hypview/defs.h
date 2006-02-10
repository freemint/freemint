/*
 * $Id$
 * 
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

#ifndef	DEFS_H
#define	DEFS_H		1
#define	VERSION "0.35.3e"

#define	LINE_BUF		512

/*	verschiedene Werte fr den Dateityp	*/
enum {
	F_LOADERROR=-2,				/*	Fehler beim Laden der Datei	*/
	F_UNKNOWN=-1,				/*	Unbekannter Dateityp	*/
	F_BINARY=0,				/*	entsprechendes Format	*/
	F_ASCII,				/*	entsprechendes Format	*/
	F_HYP,					/*	entsprechendes Format	*/
	F_LAST					/*	Letzter Typ	*/
};

typedef struct {
	long line;				/*	Zeilennummer */
	long y;
	long offset;				/*	Offset zum Anfang des Wortes (in Zeichen)	*/
	short x;				/*	x Koordinate der unteren Ecke	*/
}TEXT_POS;

typedef struct
{
	short valid;						/*	Flag, das anzeigt ob Daten vorhanden	*/
	TEXT_POS start;				/*	Block Anfang	*/
	TEXT_POS end;					/*	Block Ende	*/
}BLOCK;

#define	BLK_CHKOP			0		/*	berprfen ob Operation vorhanden	*/
#define	BLK_ASCIISAVE		1		/*	Block als ASCII unter Handle <param> sichern	*/
#define	BLK_PRINT			2		/*	Block auf Drucker ausgeben	*/

/*	Anonyme Dokumenttyp spezifische Funktionen	*/
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
	struct _document_ *next;		/*	Zeiger auf das nchste Dokument	*/
	short type;				/*	Dokumenttyp siehe F_xy	*/
	char *path;				/*	vollstndiger Zugriffspfad	*/
	char *filename;				/*	Dateiname	*/
	char *window_title;			/*	Fenster-Titel	*/
	long start_line;			/*	erste anzuzeigende Zeile	*/
	long lines;				/*	Anzahl Fenster-Zeilen	*/
	long height;
	long columns;				/*	Anzahl Fenster-Kolonnen	*/
	short buttons;				/*	Toolbar-Button-Konfiguration	*/
	void *data;				/*	Typ-spezifische Daten	*/
	short mtime,mdate;			/*	nderungszeit und -datum der Datei	*/
	WINDOW_DATA *window;			/*	Fenster, indem die Datei dargestellt wird/wurde	*/
	WINDOW_DATA *popup;			/*	Zur Zeit dargestelltes Popup	*/
	DOC_PROC1 displayProc;			/*	Funktion zur Anzeige des Dokuments	*/
	DOC_PROC1 closeProc;			/*	Funktion zum schliessen des Dokuments	*/
	DOC_GOTOPROC gotoNodeProc;		/*	Funktion zum Anspringen eines Kapitels	*/
	DOC_GETNODEPROC getNodeProc;		/*	Funktion zum Ermitteln der Kapitel-Nummer	*/
	DOC_CLICKPROC clickProc;		/*	Funktion zur Verarbeitung von Maus-Aktionen	*/
	DOC_AUTOLOCPROC autolocProc;		/*	Autolocator-Funktion	*/
	char *autolocator;			/*	Suchstring des Autolocators	*/
	DOC_GETCURSORPROC getCursorProc;	/*	Funktion zum Ermitteln der Cursor Position	*/
	BLOCK selection;			/*	Daten des aktuellen Text-Blocks	*/
	DOC_BLOCKPROC blockProc;		/*	Funktion fr Block-Operationen	*/
};
typedef struct _document_ DOCUMENT;


/*	Konvertiert Objektnummern in Bit-Werte	*/
#define BITVAL(x)	(1<<(x-4))


/*
 *		Global.c
 */
extern char path_list[];
extern char default_file[];
extern char catalog_file[];
/* [GS] 0.35.2a Start */
extern char file_extensions[];
/* Ende; alt:
extern char *file_extensions;
*/
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
/* [GS] 0.35.2a Start:*/
extern char hypfind_path[];
extern short refonly;
/* Ende */

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
/* [GS] 0.35.2c Start: */
short OpenFileNW(char *path, char *chapter, long node);
/* Ende; alt:
short OpenFileNW(char *path, char *chapter);
*/
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
/* [GS] 0.35.2a alt: verschoben nach info.c
void ProgrammInfos(void);
*/
void ReInitWindow(WINDOW_DATA *wind, DOCUMENT *doc);
short HelpWindow(WINDOW_DATA *ptr, short obj, void *data);

/*
 *		Keyrepet.c
 */
void KeyboardOnOff(void);

/*
 *		Toolbar.c
 */
/* [GS] v0.35.2e Start */
void ToolbarUpdate(DOCUMENT *doc,OBJECT *toolbar, short redraw);
/* Ende; alt:
void ToolbarUpdate(DOCUMENT *doc,OBJECT *toolbar);
*/
void ToolbarClick(DOCUMENT *doc,short obj);
void RemoveSearchBox(DOCUMENT *doc);

/*
 *		History.c
 */
typedef struct _history_
{
	struct _history_ *next;
	WINDOW_DATA *win;					/*	Zugehriges Fenster	*/
	DOCUMENT *doc;						/*	Zeiger auf Dokument	*/
	long line;							/*	1. anzuzeigende Zeile	*/
	long node;							/*	evtl. "Kapitel"-Nummer	*/
	char title;
}HISTORY;

extern HISTORY *history;			/*	Pointer auf die History-Daten	*/

void AddHistoryEntry(WINDOW_DATA *wind);
short RemoveHistoryEntry(DOCUMENT **doc,long *node,long *line);
void RemoveAllHistoryEntries(WINDOW_DATA *wind);
short CountWindowHistoryEntries(WINDOW_DATA *wind);
short CountDocumentHistoryEntries(DOCUMENT *doc);
/* [GS] 0.35.2e Start: */
void DeletLastHistory(void *entry);
void *GetLastHistory(void);
void SetLastHistory(WINDOW_DATA *the_win,void *last);
/* Ende */

/*
 *		Tools.c
 */
WINDOW_DATA *get_first_window(void);

/*
 *		Navigate.c
 */
void GotoPage(DOCUMENT *doc, long num, long line);
void GoBack(DOCUMENT *doc);
void MoreBackPopup(DOCUMENT *doc, short x, short y);
void GotoHelp(DOCUMENT *doc);
void GotoIndex(DOCUMENT *doc);
void GoThisButton(DOCUMENT *doc,short obj);

/*
 *		Autoloc.c
 */
short AutolocatorKey(DOCUMENT *doc,short ascii);
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
void MarkerSave ( DOCUMENT *doc, short num );
void MarkerShow ( DOCUMENT *doc, short num, short new_window );
void MarkerPopup ( DOCUMENT *doc, short x, short y);
void MarkerSaveToDisk ( void );
void MarkerInit( void );

/*
 *		Info.c
 */
void ProgrammInfos( DOCUMENT *doc );

/* [GS] 0.35.2a Start */
/*
 *		search.c
 */
void search_all ( char *string );
void Search (DOCUMENT *doc);

/* Ende */

#endif
