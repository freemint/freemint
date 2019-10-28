/* $Id: xterm.c,v 1.56 2019/07/10 22:00:07 tom Exp $ */

#include <vttest.h>
#include <esc.h>
#include <ttymodes.h>

#define Pause(secs) fflush(stdout); sleep(secs)

static void
check_rc(int row, int col)
{
  char *report;
  char *params;
  char expected[80];

  sprintf(expected, "%d;%dR", row, col);

  set_tty_raw(TRUE);
  set_tty_echo(FALSE);
  do_csi("6n");
  report = get_reply();
  restore_ttymodes();

  vt_move(row, 1);
  el(2);
  if ((params = skip_csi(report)) == 0
      || strcmp(params, expected) != 0) {
    printf("cursor save/restore %s, got \"%s\", expected \"%s\"",
           SHOW_FAILURE, params, expected);
  } else {
    printf("cursor save/restore %s", SHOW_SUCCESS);
  }
}

static int
test_altscrn_47(MENU_ARGS)
{
  vt_move(1, 1);
  println(the_title);
  vt_move(3, 1);
  println("Test private setmode 47 (to/from alternate screen)");
  vt_move(4, 1);
  println("The next screen will be filled with E's down to the prompt.");
  vt_move(7, 5);
  decsc();
  cup(max_lines - 2, 1);
  holdit();

  sm("?47");
  vt_move(12, 1);
  println("This message should not be here.");
  decaln();     /* fill the screen */
  vt_move(15, 7);
  decsc();
  cup(max_lines - 2, 1);
  ed(0);
  holdit();

  rm("?47");
  decrc();
  check_rc(7, 5);
  vt_move(4, 1);
  el(2);
  println("The original screen should be restored except for this line.");
  vt_move(max_lines - 2, 1);
  return MENU_HOLD;
}

static int
test_altscrn_1047(MENU_ARGS)
{
  vt_move(1, 1);
  println(the_title);
  vt_move(3, 1);
  println("Test private setmode 1047 (to/from alternate screen)");
  vt_move(4, 1);
  println("The next screen will be filled with E's down to the prompt");
  vt_move(5, 1);
  println("unless titeInhibit resource is set, or alternate-screen is disabled.");
  vt_move(7, 5);
  decsc();
  vt_move(9, 7);  /* move away from the place we saved with DECSC */
  sm("?1048");  /* this saves the cursor position */
  cup(max_lines - 2, 1);
  holdit();

  sm("?1047");
  vt_move(12, 1);
  println("This message should not be here.");
  decaln();     /* fill the screen */
  vt_move(15, 7);
  decsc();
  cup(max_lines - 2, 1);
  ed(0);
  holdit();

  rm("?1047");
  decrc();
  rm("?1048");
  check_rc(9, 7);
  vt_move(4, 1);
  el(2);
  println("The original screen should be restored except for this line");
  vt_move(max_lines - 2, 1);
  return MENU_HOLD;
}

static int
test_altscrn_1049(MENU_ARGS)
{
  vt_move(1, 1);
  println(the_title);
  vt_move(3, 1);
  println("Test private setmode 1049 (to/from alternate screen)");
  vt_move(4, 1);
  println("The next screen will be filled with E's down to the prompt.");
  vt_move(5, 1);
  println("unless titeInhibit resource is set, or alternate-screen is disabled.");
  vt_move(7, 5);
  decsc();
  cup(max_lines - 2, 1);
  holdit();     /* cursor location will be one line down */

  sm("?1049");  /* this saves the cursor location */
  vt_move(12, 1);
  println("This message should not be here.");
  decaln();     /* fill the screen */
  cup(max_lines - 2, 1);
  ed(0);
  holdit();

  rm("?1049");
  decrc();
  check_rc(max_lines - 1, 1);
  vt_move(4, 1);
  el(2);
  println("The original screen should be restored except for this line");
  vt_move(max_lines - 2, 1);
  return MENU_HOLD;
}

/*
 * Xterm implements an alternate screen, which is used to save the command-line
 * screen to restore it after running a full-screen application.
 *
 * The original scheme used separate save/restore-cursor and clear-screen
 * operations in conjunction with a toggle to/from the alternate screen
 * (private setmode 47).  Since not all users want the feature, xterm also
 * implements the titeInhibit resource to make it populate the $TERMCAP
 * variable without the ti/te (smcup/rmcup) strings which hold those sequences.
 * The limitation of titeInhibit is that it cannot work for terminfo, because
 * that information is read from a file rather than the environment.  I
 * implemented a corresponding set of strings for private setmode 1047 and 1048
 * to model the termcap behavior in terminfo.
 *
 * The behavior of the save/restore cursor operations still proved
 * unsatisfactory since users would (even in the original private setmode 47)
 * occasionally run shell programs from within their full-screen application
 * which would do a save-cursor to a different location, causing the final
 * restore-cursor to place the cursor in an unexpected location.  The private
 * setmode 1049 works around this by using a separate memory location to store
 * its version of the cursor location.
 */
static int
tst_altscrn(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
    { "Exit",                                                0 },
    { "Switch to/from alternate screen (xterm)",             test_altscrn_47 },
    { "Improved alternate screen (XFree86 xterm mode 1047)", test_altscrn_1047 },
    { "Better alternate screen (XFree86 xterm mode 1049)",   test_altscrn_1049 },
    { "",                                                    0 }
  };
  /* *INDENT-ON* */

  (void)the_title;
  do {
    vt_clear(2);
    __(title(0), println("XTERM Alternate-Screen features"));
    __(title(2), println("Choose test type:"));
  } while (menu(my_menu));
  return MENU_NOHOLD;
}

#define NUMFONTS 7

static int
tst_modify_font(MENU_ARGS)
{
  char temp[BUFSIZ];

  (void)the_title;
  vt_move(1, 1);
  println("Please enter the font name.");
  println("You can use '#' number to set fonts relative (with +/- sign) and");
  println("absolute (with a number) based on the current font's position in");
  println("the font-menu.  Examples:");
  println("    \"fixed\" to set the current font to \"fixed\"");
  println("    \"#fixed\" to set the current font to \"fixed\"");
  println("    \"#-fixed\" to set the previous font to \"fixed\"");
  println("    \"#-2 fixed\" to set the next-previous font to \"fixed\"");
  println("    \"#+fixed\" to set the following font to \"fixed\"");
  vt_move(11, 1);
  printf(">");
  inputline(temp);
  do_osc("50;%s%c", temp, BEL);
  return MENU_HOLD;
}

static int
tst_report_font(MENU_ARGS)
{
  int n;
  char *report;
  int row = 1;
  int col = 1;

  (void)the_title;
  set_tty_raw(TRUE);
  set_tty_echo(FALSE);

  vt_move(row, col);
  println("Current font:");
  ++row;

  vt_move(row, col + 6);
  do_osc("50;?%c", BEL);
  report = instr();
  row = chrprint2(report, row, col);

  ++row;
  vt_move(row, col);
  println("Absolute fonts:");
  ++row;

  for (n = 0; n < NUMFONTS; ++n) {
    vt_move(row, col);
    do_osc("50;?%d%c", n, BEL);
    report = instr();
    if (strchr(report, ';') != 0) {
      printf("  %2d: ", n);
      row = chrprint2(report, row, col);
    }
  }

  ++row;

  vt_move(row, col);
  println("Relative fonts (bell may sound):");
  ++row;

  for (n = -NUMFONTS; n < NUMFONTS; ++n) {
    vt_move(row, col);
    do_osc("50;?%c%d%c", n >= 0 ? '+' : '-', n >= 0 ? n : -n, BEL);
    report = instr();
    if (strchr(report, ';') != 0) {
      printf("  %2d: ", n);
      row = chrprint2(report, row, col);
    } else if (n >= 0) {
      break;
    }
  }

  restore_ttymodes();
  return MENU_HOLD;
}

static int
tst_font(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
    { "Exit",                                                0 },
    { "Modify font",                                         tst_modify_font },
    { "Report font(s)",                                      tst_report_font },
    { "",                                                    0 }
  };
  /* *INDENT-ON* */

  (void)the_title;
  do {
    vt_clear(2);
    __(title(0), println("XTERM Font features"));
    __(title(2), println("Choose test type:"));
  } while (menu(my_menu));
  return MENU_NOHOLD;
}

static int
test_modify_ops(MENU_ARGS)
{
  int n;
  int wide, high;
  char temp[100];

  (void)the_title;
  vt_move(1, 1);
  println("Test of Window modifying.");

  brc(2, 't');  /* iconify window */
  println("Iconify");
  Pause(2);

  brc(1, 't');  /* de-iconify window */
  println("De-Iconify");
  Pause(1);

  ed(2);
  for (n = 0; n <= 200; n += 5) {
    sprintf(temp, "Position (%d,%d)", n, n * 2);
    println(temp);
    esc("K");   /* Erase to end of line */
    brc3(3, n, n * 2, 't');
    fflush(stdout);
  }
  holdit();

  ed(2);
  brc3(3, 0, 0, 't');

  for (n = 0; n <= 200; n += 10) {
    wide = n + 20;
    high = n + 50;
    brc3(4, high, wide, 't');
    sprintf(temp, "%d x %d pixels", high, wide);
    println(temp);
    fflush(stdout);
  }
  holdit();

  ed(2);
  for (n = 0; n <= 200; n += 10) {
    high = n + 50;
    brc3(4, high, 0, 't');
    sprintf(temp, "%d x (screen-width) pixels", high);
    println(temp);
    fflush(stdout);
  }
  holdit();

  ed(2);
  for (n = 0; n <= 300; n += 10) {
    wide = n + 50;
    brc3(4, 0, wide, 't');
    sprintf(temp, "(screen-height) x %d pixels", wide);
    println(temp);
    fflush(stdout);
  }
  holdit();

  while (n >= 200) {
    wide = n + 50;
    high = 500 - n;
    brc3(4, high, wide, 't');
    sprintf(temp, "%d x %d pixels", high, wide);
    println(temp);
    fflush(stdout);
    n -= 5;
  }
  holdit();

  while (n <= 300) {
    wide = n + 50;
    high = 500 - n;
    brc3(4, high, wide, 't');
    sprintf(temp, "%d x %d pixels", high, wide);
    println(temp);
    fflush(stdout);
    n += 5;
  }
  holdit();

  ed(2);
  for (n = 5; n <= 20; n++) {
    wide = n * 4;
    high = n + 5;
    brc3(8, high, wide, 't');
    sprintf(temp, "%d x %d chars", high, wide);
    while ((int) strlen(temp) < wide - 1)
      strcat(temp, ".");
    println(temp);
    fflush(stdout);
  }
  holdit();

  ed(2);
  for (n = 5; n <= 24; n++) {
    high = n;
    brc3(8, high, 0, 't');
    sprintf(temp, "%d x (screen-width) chars", high);
    println(temp);
    fflush(stdout);
  }
  holdit();

  ed(2);
  for (n = 5; n <= 80; n++) {
    wide = n;
    brc3(8, 0, wide, 't');
    sprintf(temp, "(screen-height) x %d chars", wide);
    println(temp);
    fflush(stdout);
  }
  holdit();

  brc3(3, 200, 200, 't');
  brc3(8, 24, 80, 't');
  println("Reset to 24 x 80");

  ed(2);
  println("Lower");
  brc(6, 't');
  holdit();

  ed(2);
  println("Raise");
  brc(5, 't');
  return MENU_HOLD;
}

static int
test_report_ops(MENU_ARGS)
{
  char *report;
  int row = 3;
  int col = 10;

  (void)the_title;
  vt_move(1, 1);
  println("Test of Window reporting.");
  set_tty_raw(TRUE);
  set_tty_echo(FALSE);

  vt_move(row++, 1);
  println("Report icon label:");
  vt_move(row, col);
  brc(20, 't');
  report = instr();
  row = chrprint2(report, row, col);

  vt_move(row++, 1);
  println("Report window label:");
  vt_move(row, col);
  brc(21, 't');
  report = instr();
  row = chrprint2(report, row, col);

  vt_move(row++, 1);
  println("Report size of window (chars):");
  vt_move(row, col);
  brc(18, 't');
  report = instr();
  chrprint2(report, row++, col);

  vt_move(row++, 1);
  println("Report size of window (pixels):");
  vt_move(row, col);
  brc(14, 't');
  report = instr();
  chrprint2(report, row++, col);

  vt_move(row++, 1);
  println("Report position of window (pixels):");
  vt_move(row, col);
  brc(13, 't');
  report = instr();
  chrprint2(report, row++, col);

  vt_move(row++, 1);
  println("Report state of window (normal/iconified):");
  vt_move(row, col);
  brc(11, 't');
  report = instr();
  chrprint2(report, row, col);

  vt_move(20, 1);
  restore_ttymodes();
  return MENU_HOLD;
}

/* Set window title */
static int
test_window_name(MENU_ARGS)
{
  char temp[BUFSIZ];

  (void)the_title;
  vt_move(1, 1);
  println("Please enter the new window name.  Newer xterms may beep when setting the title.");
  inputline(temp);
  do_osc("0;%s%c", temp, BEL);
  return MENU_NOHOLD;
}

#define DATA(name,level) { name, #name, level }

/*
 * xterm's DECRQM codes are based on VT320/VT420/VT520, but there are a few
 * conflicting modes (adapted from rxvt), as well as xterm's extensions.
 * Those are reflected in the naming conventions.
 *
 * The names chosen here are short, to keep display alignment.
 */
#define XT_MSE_X10 9
#define XT_TOOLBAR 10
#define XT_CBLINK  12
#define XT_OPTBLNK 13
#define XT_XORBLNK 14
#define XT_SCRLBAR 30
#define XT_FONTSWT 35
#define XT_TEK4014 38
#define XT_80_132  40
#define XT_CURSES  41
#define XT_MARBELL 44
#define XT_REVWRAP 45
#define XT_LOGGING 46
#define XT_ALTSCRN 47
#define XT_MSE_X11 1000
#define XT_MSE_HL  1001
#define XT_MSE_BTN 1002
#define XT_MSE_ANY 1003
#define XT_MSE_WIN 1004
#define XT_MSE_UTF 1005
#define XT_MSE_SGR 1006
#define XT_ALTSCRL 1007
#define XT_TTY_OUT 1010
#define XT_SCRLKEY 1011
#define XT_IN_8BIT 1034
#define XT_NUMLOCK 1035
#define XT_METAESC 1036
#define XT_DELTDEL 1037
#define XT_ALT_ESC 1039
#define XT_KEEPSEL 1040
#define XT_SELTCLP 1041
#define XT_BELLURG 1042
#define XT_POPBELL 1043
#define XT_OLDCLIP 1044
#define XT_ALTS_OK 1046
#define XT_ALTS_47 1047
#define XT_ALTS_SC 1048
#define XT_EXTSCRN 1049
#define XT_KT_TCAP 1050
#define XT_KT_SUN  1051
#define XT_KT_HP   1052
#define XT_KT_SCO  1053
#define XT_KT_OLD  1060
#define XT_KT_PC   1061
#define RL_BTN1    2001
#define RL_BTN2    2002
#define RL_DBTN3   2003
#define RL_BRACKET 2004
#define RL_QUOTE   2005
#define RL_LIT_NL  2006

static int
tst_xterm_DECRPM(MENU_ARGS)
{
  /* *INDENT-OFF* */
  RQM_DATA dec_modes[] = { /* this list is sorted by code, not name */
    DATA( DECCKM,     3 /* cursor keys */),
    DATA( DECANM,     3 /* ANSI */),
    DATA( DECCOLM,    3 /* column */),
    DATA( DECSCLM,    3 /* scrolling */),
    DATA( DECSCNM,    3 /* screen */),
    DATA( DECOM,      3 /* origin */),
    DATA( DECAWM,     3 /* autowrap */),
    DATA( DECARM,     3 /* autorepeat */),
    DATA( XT_MSE_X10, 3 /* X10 mouse */),
    DATA( XT_TOOLBAR, 3 /* rxvt toolbar vs DECEDM */),
    DATA( DECLTM,     3 /* line transmit */),
    DATA( XT_CBLINK,  3 /* att610: Start/stop blinking cursor */),
    DATA( XT_OPTBLNK, 3 /* blink-option via menu/resource */),
    DATA( XT_XORBLNK, 3 /* enable XOR or blinking cursoor control */),
    DATA( DECEKEM,    3 /* edit key execution */),
    DATA( DECPFF,     3 /* print form feed */),
    DATA( DECPEX,     3 /* printer extent */),
    DATA( DECTCEM,    3 /* text cursor enable */),
    DATA( XT_SCRLBAR, 3 /* rxvt scrollbar */),
    DATA( DECRLM,     5 /* left-to-right */),
    DATA( XT_FONTSWT, 3 /* rxvt font-switching vs DECTEK */),
    DATA( XT_TEK4014, 3 /* Tektronix 4014 */),
    DATA( DECHEM,     5 /* Hebrew encoding */),
    DATA( XT_80_132,  3 /* 80/132 mode */),
    DATA( XT_CURSES,  3 /* curses hack */),
    DATA( DECNRCM,    3 /* national replacement character set */),
    DATA( DECGEPM,    3 /* graphics expanded print */),
    DATA( XT_MARBELL, 3 /* margin bell vs DECGPCM */),
    DATA( XT_REVWRAP, 3 /* reverse-wrap vs DECGPCS */),
    DATA( XT_LOGGING, 3 /* logging vs DECGPBM */),
    DATA( XT_ALTSCRN, 3 /* alternate screen vs DECGRPM */),
    DATA( DEC131TM,   3 /* VT131 transmit */),
    DATA( DECNAKB,    5 /* Greek/N-A Keyboard Mapping */),
    DATA( DECHCCM,    3 /* horizontal cursor coupling (disabled) */),
    DATA( DECVCCM,    3 /* vertical cursor coupling */),
    DATA( DECPCCM,    3 /* page cursor coupling */),
    DATA( DECNKM,     3 /* numeric keypad */),
    DATA( DECBKM,     3 /* backarrow key */),
    DATA( DECKBUM,    3 /* keyboard usage */),
    DATA( DECLRMM,    4 /* left/right margin mode */),
    DATA( DECXRLM,    3 /* transmit rate linking */),
    DATA( DECKPM,     4 /* keyboard positioning */),
    DATA( DECNCSM,    5 /* no clearing screen on column change */),
    DATA( DECRLCM,    5 /* right-to-left copy */),
    DATA( DECCRTSM,   5 /* CRT save */),
    DATA( DECARSM,    5 /* auto resize */),
    DATA( DECMCM,     5 /* modem control */),
    DATA( DECAAM,     5 /* auto answerback */),
    DATA( DECCANSM,   5 /* conceal answerback */),
    DATA( DECNULM,    5 /* null */),
    DATA( DECHDPXM,   5 /* half duplex */),
    DATA( DECESKM,    5 /* enable secondary keyboard language */),
    DATA( DECOSCNM,   5 /* overscan */),
    DATA( DECFWM,     5 /* framed windows */),
    DATA( DECRPL,     5 /* review previous lines */),
    DATA( DECHWUM,    5 /* host wake-up mode (CRT and energy saver) */),
    DATA( DECATCUM,   5 /* alternate text color underline */),
    DATA( DECATCBM,   5 /* alternate text color blink */),
    DATA( DECBBSM,    5 /* bold and blink style */),
    DATA( DECECM,     5 /* erase color */),
    DATA( XT_MSE_X11, 3 /* VT200 mouse */),
    DATA( XT_MSE_HL,  3 /* VT200 highlight mouse */),
    DATA( XT_MSE_BTN, 3 /* button-event mouse */),
    DATA( XT_MSE_ANY, 3 /* any-event mouse */),
    DATA( XT_MSE_WIN, 3 /* focus-event mouse */),
    DATA( XT_MSE_UTF, 3 /* UTF-8 extended mouse-coordinates */),
    DATA( XT_MSE_SGR, 3 /* SGR-style extended mouse-coordinates */),
    DATA( XT_ALTSCRL, 3 /* alternate-scroll */),
    DATA( XT_TTY_OUT, 3 /* rxvt scroll tty output */),
    DATA( XT_SCRLKEY, 3 /* rxvt scroll key */),
    DATA( XT_IN_8BIT, 3 /* input eight bits */),
    DATA( XT_NUMLOCK, 3 /* real num lock */),
    DATA( XT_METAESC, 3 /* meta sends escape */),
    DATA( XT_DELTDEL, 3 /* delete is del */),
    DATA( XT_ALT_ESC, 3 /* alt sends escape */),
    DATA( XT_KEEPSEL, 3 /* keep selection */),
    DATA( XT_SELTCLP, 3 /* select to clipboard */),
    DATA( XT_BELLURG, 3 /* bell is urgent */),
    DATA( XT_POPBELL, 3 /* pop on bell */),
    DATA( XT_ALTS_OK, 3 /* enable alt-screen switching */),
    DATA( XT_ALTS_47, 3 /* first extended alt-screen */),
    DATA( XT_ALTS_SC, 3 /* save cursor for first extended alt-screen */),
    DATA( XT_EXTSCRN, 3 /* second extended alt-screen */),
    DATA( RL_BTN1,    3 /* click1 emit Esc seq to move point*/),
    DATA( RL_BTN2,    3 /* press2 emit Esc seq to move point*/),
    DATA( RL_DBTN3,   3 /* Double click-3 deletes */),
    DATA( RL_BRACKET, 3 /* Surround paste by escapes */),
    DATA( RL_QUOTE,   3 /* Quote each char during paste */),
    DATA( RL_LIT_NL,  3 /* Paste "\n" as C-j */),
  };
  /* *INDENT-ON* */

  int old_DECRPM = set_DECRPM(3);
  int code = any_RQM(PASS_ARGS, dec_modes, TABLESIZE(dec_modes), 1);

  set_DECRPM(old_DECRPM);
  return code;
}

/*
 * Show mouse-modes, offered as an option in the mouse test-screens (since that
 * is really where these can be tested).
 */
void
show_mousemodes(void)
{
  /* *INDENT-OFF* */
  RQM_DATA mouse_modes[] = { /* this list is sorted by code, not name */
    DATA( XT_MSE_X10, 3 /* X10 mouse */),
    DATA( XT_MSE_X11, 3 /* VT200 mouse */),
    DATA( XT_MSE_HL,  3 /* VT200 highlight mouse */),
    DATA( XT_MSE_BTN, 3 /* button-event mouse */),
    DATA( XT_MSE_ANY, 3 /* any-event mouse */),
    DATA( XT_MSE_WIN, 3 /* focus-event mouse */),
    DATA( XT_MSE_UTF, 3 /* UTF-8 extended mouse-coordinates */),
    DATA( XT_MSE_SGR, 3 /* SGR-style extended mouse-coordinates */),
  };
  /* *INDENT-ON* */

  int old_DECRPM = set_DECRPM(3);
  vt_clear(2);
  (void) any_RQM("mouse modes", mouse_modes, TABLESIZE(mouse_modes), 1);
  set_DECRPM(old_DECRPM);
  holdit();
}

#undef DATA
static int
tst_xterm_reports(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
    { "Exit",                                                0 },
    { "Test VT520 features",                                 tst_vt520_reports },
    { "Request Mode (DECRQM)/Report Mode (DECRPM)",          tst_xterm_DECRPM },
    { "",                                                    0 }
  };
  /* *INDENT-ON* */

  (void)the_title;
  do {
    vt_clear(2);
    __(title(0), println("XTERM miscellaneous reports"));
    __(title(2), println("Choose test type:"));
  } while (menu(my_menu));
  return MENU_NOHOLD;
}

/*
 * xterm (and derived programs such as hpterm, dtterm, rxvt) are the most
 * widely used vt100 near-compatible terminal emulators (other than modem
 * programs).  dtterm emulates a vt220, as does XFree86 xterm.  DECterm
 * emulates a vt320.
 */
int
tst_xterm(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
    { "Exit",                                                0 },
    { "Test VT520 features",                                 tst_vt520 },
    { "Test reporting functions",                            tst_xterm_reports },
    { "Set window title",                                    test_window_name },
    { "Font features",                                       tst_font },
    { "Mouse features",                                      tst_mouse },
    { "Tektronix 4014 features",                             tst_tek4014 },
    { "Alternate-Screen features (xterm)",                   tst_altscrn },
    { "Window modify-operations (dtterm)",                   test_modify_ops },
    { "Window report-operations (dtterm)",                   test_report_ops },
    { "",                                                    0 }
  };
  /* *INDENT-ON* */

  (void)the_title;
  do {
    vt_clear(2);
    __(title(0), println("XTERM special features"));
    __(title(2), println("Choose test type:"));
  } while (menu(my_menu));
  return MENU_NOHOLD;
}
