/*
 * Reference:  VT520/VT525 Video Terminal Programmer Information
 *             (EK-VT520-RM.A01)
 */
#include <vttest.h>
#include <draw.h>
#include <esc.h>
#include <ttymodes.h>

/******************************************************************************/

static struct {
  int code;
  const char *text;
} tbl_decscusr[] = {

  {
    1, "The cursor should be a blinking rectangle"
  },
  {
    0, "The cursor should be a blinking rectangle"
  },
  {
    2, "The cursor should be a nonblinking rectangle"
  },
  {
    3, "The cursor should be a blinking underline"
  },
  {
    4, "The cursor should be a nonblinking underline"
  },
  {
    5, "xterm: The cursor should be a blinking vertical bar"
  },
  {
    6, "xterm: The cursor should be a nonblinking vertical bar"
  },
  {
    2, "The cursor should be a rectangle again"
  }
};

/******************************************************************************/
static int
tst_vt520_cursor(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Test VT420 features",                               tst_vt420_cursor },
      { origin_mode_mesg,                                    toggle_DECOM },
      { lrmm_mesg,                                           toggle_LRMM },
      { tb_marg_mesg,                                        toggle_STBM },
      { lr_marg_mesg,                                        toggle_SLRM },
      { txt_override_color,                                  toggle_color_mode, },
      { "Test Character-Position-Absolute (HPA)",            tst_HPA },
      { "Test Cursor-Back-Tab (CBT)",                        tst_CBT },
      { "Test Cursor-Character-Absolute (CHA)",              tst_CHA },
      { "Test Cursor-Horizontal-Index (CHT)",                tst_CHT },
      { "Test Horizontal-Position-Relative (HPR)",           tst_HPR },
      { "Test Line-Position-Absolute (VPA)",                 tst_VPA },
      { "Test Next-Line (CNL)",                              tst_CNL },
      { "Test Previous-Line (CPL)",                          tst_CPL },
      { "Test Vertical-Position-Relative (VPR)",             tst_VPR },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  setup_vt420_cursor(PASS_ARGS);

  do {
    vt_clear(2);
    __(title(0), printf("VT520 Cursor-Movement"));
    __(title(2), println("Choose test type:"));
    menus_vt420_cursor();
  } while (menu(my_menu));

  finish_vt420_cursor(PASS_ARGS);

  return MENU_NOHOLD;
}

/******************************************************************************/
/*
 * VT500 & up
 *
 * Test if terminal can control whether the screen is cleared when changing
 * DECCOLM.
 */
static int
tst_DECNCSM(MENU_ARGS)
{
  int last = max_lines - 4;
  char temp[80];

  (void)the_title;
  decaln();
  deccolm(FALSE);
  vt_move(last, 1);
  ruler(last, min_cols);
  vt_clear(0);
  sprintf(temp, "Screen should be cleared (%d-columns)", min_cols);
  println(temp);
  holdit();

  decaln();
  deccolm(TRUE);
  vt_move(last, 1);
  ruler(last, max_cols);
  vt_clear(0);
  sprintf(temp, "Screen should be cleared (%d-columns)", max_cols);
  println(temp);
  holdit();

  decncsm(TRUE);

  decaln();
  deccolm(FALSE);
  vt_move(last, 1);
  ruler(last, min_cols);
  vt_clear(0);
  sprintf(temp, "Screen should be filled (%d-columns)", min_cols);
  println(temp);
  holdit();

  decaln();
  deccolm(TRUE);
  vt_move(last, 1);
  ruler(last, max_cols);
  vt_clear(0);
  sprintf(temp, "Screen should be filled (%d of %d-columns)", min_cols, max_cols);
  println(temp);
  holdit();

  decncsm(FALSE);
  deccolm(FALSE);
  vt_move(last, 1);
  ruler(last, min_cols);
  vt_clear(0);
  sprintf(temp, "Screen should be cleared (%d-columns)", min_cols);
  println(temp);

  return MENU_HOLD;
}

/******************************************************************************/

static void
decscusr(int parm)
{
  do_csi("%d q", parm);
}

/*
 * VT500 & up
 *
 * Test if the terminal can change the cursor's appearance
 */
static int
tst_DECSCUSR(MENU_ARGS)
{
  size_t n;

  (void)the_title;
  vt_move(1, 1);
  for (n = 0; n < TABLESIZE(tbl_decscusr); ++n) {
    if (n != 0)
      holdit();
    decscusr(tbl_decscusr[n].code);
    println(tbl_decscusr[n].text);
  }
  return MENU_HOLD;
}

/******************************************************************************/

static int
rpt_DECCRTST(MENU_ARGS)
{
  return any_decrqss(the_title, "-q");
}

static int
rpt_DECDLDA(MENU_ARGS)
{
  return any_decrqss(the_title, ",z");
}

static int
rpt_DECSEST(MENU_ARGS)
{
  return any_decrqss(the_title, "-z");
}

static int
rpt_DECSCP(MENU_ARGS)
{
  return any_decrqss(the_title, "*u");
}

static int
rpt_DECSCS(MENU_ARGS)
{
  return any_decrqss(the_title, "*r");
}

static int
rpt_DECSDPT(MENU_ARGS)
{
  return any_decrqss(the_title, "(p");
}

static int
rpt_DECSDDT(MENU_ARGS)
{
  return any_decrqss(the_title, "$p");
}

static int
rpt_DECSFC(MENU_ARGS)
{
  return any_decrqss(the_title, "*s");
}

static int
rpt_DECSPRTT(MENU_ARGS)
{
  return any_decrqss(the_title, "$s");
}

static int
rpt_DECSPPCS(MENU_ARGS)
{
  return any_decrqss(the_title, "*p");
}

static int
rpt_DECSSL(MENU_ARGS)
{
  return any_decrqss(the_title, "p");
}

static int
rpt_DECSPMA(MENU_ARGS)
{
  return any_decrqss(the_title, ",x");
}

static int
rpt_DECSPP(MENU_ARGS)
{
  return any_decrqss(the_title, "+w");
}

static int
rpt_DECSSCLS(MENU_ARGS)
{
  return any_decrqss(the_title, " p");
}

static int
rpt_DECSTRL(MENU_ARGS)
{
  return any_decrqss(the_title, "\"u");
}

/*
 * This list is separated from the main one, to keep the menu size fitting on
 * a 24x80 screen.
 */
static int
tst_VT520_hardware_DECRQSS(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Test VT420 features (DECRQSS)",                     tst_vt420_DECRQSS },
      { "CRT Saver Timing (DECCRTST)",                       rpt_DECCRTST },
      { "Down Line Load Allocation (DECDLDA)",               rpt_DECDLDA },
      { "Energy Save Timing (DECSEST)",                      rpt_DECSEST },
      { "Select Communication Port (DECSCP)",                rpt_DECSCP },
      { "Select Communication Speed (DECSCS)",               rpt_DECSCS },
      { "Select Digital Printed Data Type (DECSDPT)",        rpt_DECSDPT },
      { "Select Disconnect Delay Time (DECSDDT)",            rpt_DECSDDT },
      { "Select Flow Control Type (DECSFC)",                 rpt_DECSFC },
      { "Select Printer Type (DECSPRTT)",                    rpt_DECSPRTT },
      { "Select ProPrinter Character Set (DECSPPCS)",        rpt_DECSPPCS },
      { "Select Set-Up Language (DECSSL)",                   rpt_DECSSL },
      { "Session Page Memory Allocation (DECSPMA)",          rpt_DECSPMA },
      { "Set Port Parameter (DECSPP)",                       rpt_DECSPP },
      { "Set Scroll Speed (DECSSCLS)",                       rpt_DECSSCLS },
      { "Set Transmit Rate Limit (DECSTRL)",                 rpt_DECSTRL },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  (void)the_title;
  do {
    vt_clear(2);
    __(title(0), printf("VT520 Status-Strings Reports (Hardware-oriented)"));
    __(title(2), println("Choose test type:"));
  } while (menu(my_menu));
  return MENU_NOHOLD;
}

/******************************************************************************/

static int
rpt_DECATR(MENU_ARGS)
{
  return any_decrqss(the_title, ",}");
}

static int
rpt_DECAC(MENU_ARGS)
{
  return any_decrqss(the_title, ",|");
}

static int
rpt_DECARR(MENU_ARGS)
{
  return any_decrqss(the_title, "-p");
}

static int
rpt_DECSTGLT(MENU_ARGS)
{
  return any_decrqss(the_title, "){");
}

static int
rpt_DECSZS(MENU_ARGS)
{
  return any_decrqss(the_title, ",{");
}

static int
rpt_DECSCUSR(MENU_ARGS)
{
  size_t n;

  (void)the_title;
  vt_move(1, 1);
  for (n = 0; n < TABLESIZE(tbl_decscusr); ++n) {
    char expected[10];
    if (n != 0)
      holdit();
    vt_clear(2);
    decscusr(tbl_decscusr[n].code);
    sprintf(expected, "%d", tbl_decscusr[n].code ? tbl_decscusr[n].code : 1);
    (void) any_decrqss2(tbl_decscusr[n].text, " q", expected);
  }
  return MENU_HOLD;
}

static int
rpt_DECSKCV(MENU_ARGS)
{
  return any_decrqss(the_title, " r");
}

static int
rpt_DECSLCK(MENU_ARGS)
{
  return any_decrqss(the_title, " v");
}

static int
rpt_DECSMBV(MENU_ARGS)
{
  return any_decrqss(the_title, " u");
}

static int
rpt_DECSWBV(MENU_ARGS)
{
  return any_decrqss(the_title, " t");
}

static int
rpt_DECTME(MENU_ARGS)
{
  return any_decrqss(the_title, " ~");
}

static int
tst_VT520_DECRQSS(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Test VT420 features (DECRQSS)",                     tst_vt420_DECRQSS },
      { "Test Hardware-oriented features",                   tst_VT520_hardware_DECRQSS },
      { "Alternate Text Color (DECATR)",                     rpt_DECATR },
      { "Assign Color (DECAC)",                              rpt_DECAC },
      { "Select Auto Repeat Rate (DECARR)",                  rpt_DECARR },
      { "Select Color Lookup Table (DECSTGLT)",              rpt_DECSTGLT },
      { "Select Zero Symbol (DECSZS)",                       rpt_DECSZS },
      { "Set Cursor Style (DECSCUSR)",                       rpt_DECSCUSR },
      { "Set Key Click Volume (DECSKCV)",                    rpt_DECSKCV },
      { "Set Lock Key Style (DECSLCK)",                      rpt_DECSLCK },
      { "Set Margin Bell Volume (DECSMBV)",                  rpt_DECSMBV },
      { "Set Warning Bell Volume (DECSWBV)",                 rpt_DECSWBV },
      { "Terminal Mode Emulation (DECTME)",                  rpt_DECTME },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  (void)the_title;
  do {
    vt_clear(2);
    __(title(0), printf("VT520 Status-Strings Reports"));
    __(title(2), println("Choose test type:"));
  } while (menu(my_menu));
  return MENU_NOHOLD;
}

/******************************************************************************/

static int
tst_VT520_report_presentation(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Test VT420 features",                               tst_vt420_report_presentation },
      { "Request Mode (DECRQM)/Report Mode (DECRPM)",        tst_DECRPM },
      { "Status-String Report (DECRQSS)",                    tst_VT520_DECRQSS },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  int old_DECRPM = set_DECRPM(5);

  (void)the_title;
  do {
    vt_clear(2);
    __(title(0), printf("VT520 Presentation State Reports"));
    __(title(2), println("Choose test type:"));
  } while (menu(my_menu));
  set_DECRPM(old_DECRPM);
  return MENU_NOHOLD;
}

int
tst_vt520_reports(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Test VT420 features",                               tst_vt420_reports },
      { "Test Presentation State Reports",                   tst_VT520_report_presentation },
      { "Test Device Status Reports (DSR)",                  tst_vt420_device_status },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  (void)the_title;
  do {
    vt_clear(2);
    __(title(0), printf("VT520 Reports"));
    __(title(2), println("Choose test type:"));
  } while (menu(my_menu));
  return MENU_NOHOLD;
}

/******************************************************************************/

static int
tst_vt520_screen(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Test No Clear on Column Change (DECNCSM)",          tst_DECNCSM },
      { "Test Set Cursor Style (DECSCUSR)",                  tst_DECSCUSR },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  (void)the_title;
  do {
    vt_clear(2);
    __(title(0), printf("VT520 Screen-Display Tests"));
    __(title(2), println("Choose test type:"));
  } while (menu(my_menu));
  return MENU_NOHOLD;
}

/******************************************************************************/

/*
 * These apply only to VT500's & above.
 */
int
tst_vt520(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Test VT420 features",                               tst_vt420 },
      { "Test cursor-movement",                              tst_vt520_cursor },
      { "Test editing sequences",                            not_impl },
      { "Test keyboard-control",                             not_impl },
      { "Test reporting functions",                          tst_vt520_reports },
      { "Test screen-display functions",                     tst_vt520_screen },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  (void)the_title;
  do {
    vt_clear(2);
    __(title(0), printf("VT520 Tests"));
    __(title(2), println("Choose test type:"));
  } while (menu(my_menu));
  return MENU_NOHOLD;
}
