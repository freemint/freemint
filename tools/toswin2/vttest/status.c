/* $Id: status.c,v 1.7 2012/04/12 14:57:28 tom Exp $ */

#include <vttest.h>
#include <esc.h>
#include <ttymodes.h>

static void
restore_status(void)
{
  decsasd(0);   /* main display */
  decssdt(1);   /* indicator (default) */
  restore_ttymodes();
}

static int
simple_statusline(MENU_ARGS)
{
  static char text[] = "TEXT IN THE STATUS LINE";

  (void)the_title;
  vt_move(1, 1);
  println("This is a simple test of the status-line");
  println("");

  decssdt(2);
  decsasd(1);
  printf("%s", text);
  decsasd(0);
  printf("There should be %s\r\n", text);
  holdit();

  decssdt(0);
  println("There should be no status line");
  holdit();

  decssdt(1);
  println("The status line should be normal (i.e., indicator)");
  holdit();

  restore_status();
  return MENU_NOHOLD;
}

static int
SGR_statusline(MENU_ARGS)
{
  (void)the_title;
  vt_move(1, 1);
  println("This test writes SGR controls to the status-line");
  holdit();

  decssdt(2);
  decsasd(1);

  el(2);
  cup(1, 1);
  sgr("1");
  printf("BOLD text ");
  sgr("0");
  printf("NORMAL text ");

  decsasd(0);
  holdit();

  restore_status();
  holdit();

  restore_status();
  return MENU_NOHOLD;
}

/* VT200 & up
 *
 *  CSI Ps $ }     DECSASD         Select active status display
 *         Ps = 0 select main display
 *         Ps = 1 select status line
 *         Moves cursor to selected display area.  This command will be ignored
 *         unless the status line has been enabled by CSI 2 $ ~.  When the
 *         status line has been selected cursor remains there until the main
 *         display is reselected by CSI 0 $ }.
 *
 *  CSI Ps $ ~     DECSSDT         Select Status Line Type
 *                         Ps      meaning
 *                         0       no status line (empty)
 *                         1       indicator line
 *                         2       host-writable line
 */
int
tst_statusline(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Simple Status line Test",                           simple_statusline },
      { "Test Graphic-Rendition in Status line",             SGR_statusline },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  do {
    vt_clear(2);
    __(title(0), println(the_title));
    __(title(2), println("Choose test type:"));
  } while (menu(my_menu));
  decssdt(0);
  return MENU_NOHOLD;
}
