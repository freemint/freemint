#include "xa_defs.h"
/*
 * Output-strings
 */

char *xa_strings[] = {
 /* window-titles */
	"About      ",
	"Task Manager           ",
	"System window & Alerts log           ",
/* system-window*/
	"Applications",
	"Alerts          ",
	"Environment  ",
	"System  ",
	"Video:%dx%dx%d,%d colours, format: %s ",
	"MEMPROT:%s",
	"OFF",
	"ON",
	"Switch keyboard, current:",
	"unknown  ",
/* alert-text */
"[2][Quit All][Cancel|OK]     ",
"[2][Leave XaAES][Cancel|OK]   ",
"[2][Restart XaAES][Cancel|OK]    ",
"[2][Halt System][Cancel|OK]     ",
"[1][Snapper could not save snap!|ERROR: %d][OK]                     ",
"[1][Cannot snap topwindow as|parts of it are offscreen!][OK]                                                   ",
"[1]['xaaesnap' process not found.|Start 'xaaesnap.prg' and try again|or define snapshot in xaaes.cnf][OK]                            ",
"[2][What do you want to snap?][Block|Full screen|Top Window|Cancel]          ",
"Launch Program     ",
"$SDMASTER is not a valid program: %s (use the taskmanager)                      ",
"Could not load %s.tbl from %s (%ld)         ",
"Cannot terminate AES system proccesses!               ",
"Cannot kill XaAES!                    ",
"XaAES: No AES Parameter Block, returning.          ",
"XaAES: Non-AES process issued AES system call %i, killing it!             ",
"%s: Validate OBTREE for %s failed, object ptr = %lx, killing it!             ",
"attach_extension for %u failed, out of memory?           ",
"umalloc for %u failed, out of memory?           ",
"XaAES: Print dialogs unavailable with current VDI.         ",
"launch: Cannot enter single-task-mode. Already in single-task-mode: %s(%d).        ",
"XaAES: %s%s, client with no globl_ptr, killing it!          ",
"XaAES: AES shell already running!                    ",
"XaAES: No AES shell set, see 'shell =' configuration variable in xaaes.cnf.                 ",

"SPACE",
"DELETE",

"  Clients \3",

/* help-file */
"xa_help.txt",
	0
};

char *wctxt_main_txt[] =
{
 "\255Windows     ",
   "\1Advanced    ",
 "\255To desktop  ",
   "\2Close       ",
   "\3Hide        ",
   "\2Iconify     ",
   "\2Shade       ",
 "\255Move        ",
 "\255Resize      ",
 "\255Quit        ",
 "\255Kill        ",
 "",
	"\255Keep above others           ",
	"\255Keep below others           ",
	"\255Toolbox attribute           ",
	"\255Deny keyboard focus         ",
	"",
	"\255This                      ",
	"\255All                       ",
	"\255All others                ",
	"\255Restore all               ",
	"\255Restore all others        ",
	"",
	"\255This window               ",
	"\255Application               ",
	"\255Other apps                ",
	"\255Show other apps           ",
	"","",
	0
};

//const char mnu_clientlistname[] =


#if XAAES_RELEASE
#define FN_CRED	0
#else
#define FN_CRED	0
#endif

char *about_lines[] =
{
  /*          1         2         3         4         5         6
     123456789012345678901234567890123456789012345678901234567890 */
#if FN_CRED
	"",
	"           <u>Dedicated to <i>Frank Naumann </i></u>\xbb",
	"<u>                                                                  </u>",
#endif
	"",
	"Part of FreeMiNT ("SPAREMINT_URL").       ",
	"",
	"The terms of the <b>GPL version 2</b> or later apply.            ",
	"",
	0
};


char **trans_strings[] = {
	about_lines,
	wctxt_main_txt,
	xa_strings,
	0
};
