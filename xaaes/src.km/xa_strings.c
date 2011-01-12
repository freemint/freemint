/*
 * Output-strings (to be moved into the rsc-file)
 */

char *xa_strings[] = {
 /* window-titles */
	"  About  ",
	" Task Manager ",
	" System window & Alerts log",
/* system-window*/
	"Applications",
	"Alerts    ",
	"Environment",
	"System",
	"Video:%dx%dx%d,%d colours, format: %s",
	"MEMPROT:%s",
	"OFF",
	"ON",
/* alert-text */
"[2][Quit All][Cancel|Ok]",
"[2][Leave XaAES][Cancel|Ok]     ",
"[1][ Snapper could not save snap! | ERROR: %d ][ Ok ]",
"[1][Cannot snap topwindow as | parts of it is offscreen!][OK]",
"[1]['xaaesnap' process not found.|Start 'xaaesnap.prg' and try again|or define snapshot in xaaes.cnf][OK]",
"[2][ What do you want to snap?][ Block | Full screen | Top Window | Cancel ]",
" Launch Program ",
"xa_help.txt",
"$SDMASTER is not a valid program: %s (use the taskmanager)",
"could not load %s.tbl from %s (%ld)",
	0
};


#if 0
/* about-window */
char *AB_GPL = 	"The terms of the <b>GPL version 2</b> or later apply.";
char *AB_MINT = 	"Part of freemint ("SPAREMINT_URL").";
#endif

extern char *wctxt_main_txt[];
extern char *about_lines[];

char **trans_strings[] = {
	about_lines,
	wctxt_main_txt,
	xa_strings,
	0
};


