/* $Id: vt220.c,v 1.25 2018/07/26 00:39:55 tom Exp $ */

/*
 * Reference:  VT220 Programmer Pocket Guide (EK-VT220-HR-002).
 * Reference:  VT220 Program Reference Manual (EK-VT220-RM-002).
 */
#include <vttest.h>
#include <ttymodes.h>
#include <esc.h>

int
any_DSR(MENU_ARGS, const char *text, void (*explain) (char *report))
{
  int row, col;
  char *report;
  unsigned pmode = (unsigned) ((*text == '?') ? 1 : 0);

  vt_move(1, 1);
  printf("Testing DSR: %s\n", the_title);

  set_tty_raw(TRUE);
  set_tty_echo(FALSE);

  do_csi("%s", text);
  report = get_reply();
  vt_move(row = 3, col = 10);
  chrprint2(report, row, col);
  if ((report = skip_csi(report)) != 0
      && strlen(report) > (1 + pmode)
      && (!pmode || (*report++ == '?'))) {
    if (explain != 0)
      (*explain) (report);
    else
      show_result(SHOW_SUCCESS);
  } else {
    show_result(SHOW_FAILURE);
  }

  restore_ttymodes();
  vt_move(max_lines - 1, 1);
  return MENU_HOLD;
}

static void
report_ok(const char *ref, const char *tst)
{
  if ((tst = skip_csi_2(tst)) == 0)
    tst = "?";
  show_result(!strcmp(ref, tst) ? SHOW_SUCCESS : SHOW_FAILURE);
}

/*
 * Request  CSI ? 26 n            keyboard dialect
 * Response CSI ? 27; Ps n
 */
static void
show_KeyboardStatus(char *report)
{
  int pos = 0;
  int code;
  int save;
  const char *show = SHOW_FAILURE;

  if ((code = scanto(report, &pos, ';')) == 27
      && (code = scan_any(report, &pos, 'n')) != 0) {
    /* *INDENT-OFF* */
    switch(code) {
    case  1:  show = "North American/ASCII"; break;
    case  2:  show = "British";              break;
    case  3:  show = "Flemish";              break;
    case  4:  show = "French Canadian";      break;
    case  5:  show = "Danish";               break;
    case  6:  show = "Finnish";              break;
    case  7:  show = "German";               break;
    case  8:  show = "Dutch";                break;
    case  9:  show = "Italian";              break;
    case 10:  show = "Swiss (French)";       break;
    case 11:  show = "Swiss (German)";       break;
    case 12:  show = "Swedish";              break;
    case 13:  show = "Norwegian/Danish";     break;
    case 14:  show = "French/Belgian";       break;
    case 15:  show = "Spanish Int.";         break;
    case 16:  show = "Portugese";            break; /* vt3XX */
    case 19:  show = "Hebrew";               break; /* vt5XX: kermit says 14 */
    case 22:  show = "Greek";                break; /* vt5XX */
    case 28:  show = "Canadian (English)";   break; /* vt4XX */
    case 29:  show = "Turkish Q/Turkish";    break; /* vt5XX */
    case 30:  show = "Turkish F/Turkish";    break; /* vt5XX */
    case 31:  show = "Hungarian";            break; /* vt5XX */
    case 32:  show = "Spanish National";     break; /* vt4XX in PC mode */
    case 33:  show = "Slovak";               break; /* vt5XX */
    case 34:  show = "Czech";                break; /* vt5XX */
    case 35:  show = "Polish";               break; /* vt5XX */
    case 36:  show = "Romanian";             break; /* vt5XX */
    case 38:  show = "SCS";                  break; /* vt5XX */
    case 39:  show = "Russian";              break; /* vt5XX */
    case 40:  show = "Latin American";       break; /* vt5XX */
    default:  show = "unknown";
    }
    /* *INDENT-ON* */

  }
  show_result("%s", show);

  /* vt420 implements additional parameters past those reported by the VT220 */
  save = pos;
  code = scan_any(report, &pos, 'n');
  if (save != pos) {
    vt_move(5, 10);
    /* *INDENT-OFF* */
    switch(code) {
    case 0:  show = "keyboard ready"; break;
    case 3:  show = "no keyboard";    break;
    case 8:  show = "keyboard busy";  break;
    default: show = "unknown keyboard status";
    }
    /* *INDENT-ON* */

    show_result("%s", show);

    vt_move(6, 10);
    /* *INDENT-OFF* */
    switch (scan_any(report, &pos, 'n')) {
    case 0:  show = "LK201"; break;
    case 1:  show = "LK401"; break;
    default: show = "unknown keyboard type";
    }
    /* *INDENT-ON* */

    show_result("%s", show);
  }
}

static void
show_PrinterStatus(char *report)
{
  int pos = 0;
  int code = scanto(report, &pos, 'n');
  const char *show;
  /* *INDENT-OFF* */
  switch (code) {
  case 13: show = "No printer";        break;
  case 10: show = "Printer ready";     break;
  case 11: show = "Printer not ready"; break;
  case 18: show = "Printer busy";      break;
  case 19: show = "Printer assigned to other session"; break;
  default: show = SHOW_FAILURE;
  }
  /* *INDENT-ON* */

  show_result("%s", show);
}

static void
show_UDK_Status(char *report)
{
  int pos = 0;
  int code = scanto(report, &pos, 'n');
  const char *show;
  /* *INDENT-OFF* */
  switch(code) {
  case 20: show = "UDKs unlocked"; break;
  case 21: show = "UDKs locked";   break;
  default: show = SHOW_FAILURE;
  }
  /* *INDENT-ON* */

  show_result("%s", show);
}

/* VT220 & up.
 */
static int
tst_S8C1T(MENU_ARGS)
{
  int flag = input_8bits;
  int pass;
  char temp[80];

  vt_move(1, 1);
  println(the_title);

  vt_move(5, 1);
  println("This tests the VT200+ control sequence to direct the terminal to emit 8-bit");
  println("control-sequences instead of <esc> sequences.");

  set_tty_raw(TRUE);
  set_tty_echo(FALSE);

  for (pass = 0; pass < 2; pass++) {
    char *report;
    int row, col;

    flag = !flag;
    s8c1t(flag);
    cup(1, 1);
    dsr(6);
    report = instr();
    vt_move(row = 10 + pass * 3, col = 1);
    sprintf(temp, "8-bit controls %s:", STR_ENABLED(flag));
    printf("%s", temp);
    chrprint2(report, row, col + (int) strlen(temp));
    report_ok("1;1R", report);
  }

  restore_ttymodes();
  vt_move(max_lines - 1, 1);
  return MENU_HOLD;
}

/*
 * Test DEC's selective-erase (set-protected area) by drawing a box of
 * *'s that will remain, and a big X of *'s that gets cleared..
 */
static int
tst_DECSCA(MENU_ARGS)
{
  int i, j, pass;
  int tmar = 5;
  int bmar = max_lines - 8;
  int lmar = 20;
  int rmar = min_cols - lmar;

  (void)the_title;
  for (pass = 0; pass < 2; pass++) {
    if (pass == 0)
      decsca(1);
    for (i = tmar; i <= bmar; i++) {
      cup(i, lmar);
      for (j = lmar; j <= rmar; j++) {
        printf("*");
      }
    }
    if (pass == 0) {
      decsca(0);

      for (j = 0; j <= 2; j++) {
        for (i = 1; i < tmar; i++) {
          cup(i, lmar - tmar + (i + j));
          printf("*");
          cup(i, rmar + tmar - (i + j));
          printf("*");
        }
        for (i = bmar + 1; i < max_lines; i++) {
          cup(i, lmar + bmar - i + j);
          printf("*");
          cup(i, rmar - bmar + i - j);
          printf("*");
        }
        cup(max_lines / 2, min_cols / 2);
        decsed(j);
      }

      for (i = rmar + 1; i <= min_cols; i++) {
        cup(tmar, i);
        printf("*");
        cup(max_lines / 2, i);
        printf("*");
      }
      cup(max_lines / 2, min_cols / 2);
      decsel(0);  /* after the cursor */

      for (i = 1; i < lmar; i++) {
        cup(tmar, i);
        printf("*");
        cup(max_lines / 2, i);
        printf("*");
      }
      cup(max_lines / 2, min_cols / 2);
      decsel(1);  /* before the cursor */

      cup(tmar, min_cols / 2);
      decsel(2);  /* the whole line */

      vt_move(max_lines - 3, 1);
      vt_clear(0);
      println("If your terminal supports DEC protected areas (DECSCA, DECSED, DECSEL),");
      println("there will be an solid box made of *'s in the middle of the screen.");
      holdit();
    }
  }
  return MENU_NOHOLD;
}

/*
 * VT220 & up
 *
 * Test if the terminal can make the cursor invisible
 */
static int
tst_DECTCEM(MENU_ARGS)
{
  (void)the_title;
  vt_move(1, 1);
  rm("?25");
  println("The cursor should be invisible");
  holdit();
  sm("?25");
  println("The cursor should be visible again");
  return MENU_HOLD;
}

static int
tst_DECUDK(MENU_ARGS)
{
  int key;
  /* *INDENT-OFF* */
  static struct {
    int code;
    const char *name;
  } keytable[] = {
    /* xterm programs these: */
    { 11, "F1" },
    { 12, "F2" },
    { 13, "F3" },
    { 14, "F4" },
    { 15, "F5" },
    /* vt420 programs these: */
    { 17, "F6" },
    { 18, "F7" },
    { 19, "F8" },
    { 20, "F9" },
    { 21, "F10" },
    { 23, "F11" },
    { 24, "F12" },
    { 25, "F13" },
    { 26, "F14" },
    { 28, "F15" },
    { 29, "F16" },
    { 31, "F17" },
    { 32, "F18" },
    { 33, "F19" },
    { 34, "F20" } };
  /* *INDENT-ON* */

  for (key = 0; key < TABLESIZE(keytable); key++) {
    char temp[80];
    const char *s;
    temp[0] = '\0';
    for (s = keytable[key].name; *s; s++)
      sprintf(temp + strlen(temp), "%02x", *s & 0xff);
    do_dcs("1;1|%d/%s", keytable[key].code, temp);
  }

  vt_move(1, 1);
  println(the_title);
  println("Press 'q' to quit.  Function keys should echo their labels.");
  println("(On a DEC terminal you must press SHIFT as well).");

  set_tty_raw(TRUE);
  set_tty_echo(FALSE);

  for (;;) {
    int row, col;
    char *report = instr();

    if (*report == 'q')
      break;
    vt_move(row = 5, col = 10);
    vt_clear(0);
    chrprint2(report, row, col);
  }

  do_dcs("0");  /* clear all keys */

  restore_ttymodes();
  vt_move(max_lines - 1, 1);
  return MENU_HOLD;
}

/* vt220 & up */
int
tst_DSR_keyboard(MENU_ARGS)
{
  return any_DSR(PASS_ARGS, "?26n", show_KeyboardStatus);
}

int
tst_DSR_printer(MENU_ARGS)
{
  return any_DSR(PASS_ARGS, "?15n", show_PrinterStatus);
}

int
tst_DSR_userkeys(MENU_ARGS)
{
  return any_DSR(PASS_ARGS, "?25n", show_UDK_Status);
}

static void
show_OperatingStatus(char *report)
{
  int pos = 0;
  int Ps1 = scan_any(report, &pos, 'n');
  int Ps2 = scanto(report, &pos, 'n');
  const char *show;

  switch (Ps1) {
  case 0:
    show = "Terminal is in good operating condition";
    break;
  case 3:
    show = "Terminal has a malfunction";
    break;
  default:
    show = SHOW_FAILURE;
  }
  show_result(show, Ps2);
}

static int
tst_DSR_operating_status(MENU_ARGS)
{
  return any_DSR(PASS_ARGS, "5n", show_OperatingStatus);
}

/*
 * VT200 and up
 *
 * Test to ensure that 'ech' (erase character) is honored, with no parameter,
 * explicit parameter, and longer than the screen width (to ensure that the
 * terminal doesn't try to wrap-around the erasure).
 */
static int
tst_ECH(MENU_ARGS)
{
  int i;
  int last = max_lines - 4;

  (void)the_title;
  decaln();
  for (i = 1; i <= max_lines; i++) {
    cup(i, min_cols - i - 2);
    do_csi("X");  /* make sure default-parameter works */
    cup(i, min_cols - i - 1);
    printf("*");
    ech(min_cols);
    printf("*");  /* this should be adjacent, in the upper-right corner */
  }

  vt_move(last, 1);
  vt_clear(0);

  vt_move(last, min_cols - (last + 10));
  println("diagonal: ^^ (clear)");
  println("ECH test: there should be E's with a gap before diagonal of **'s");
  println("The lower-right diagonal region should be cleared.  Nothing else.");
  return MENU_HOLD;
}

/******************************************************************************/

int
tst_vt220_device_status(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Test Keyboard Status",                              tst_DSR_keyboard },
      { "Test Operating Status",                             tst_DSR_operating_status },
      { "Test Printer Status",                               tst_DSR_printer },
      { "Test UDK Status",                                   tst_DSR_userkeys },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  (void)the_title;
  do {
    vt_clear(2);
    title(0);
    printf("VT220 Device Status Reports");
    title(2);
    println("Choose test type:");
  } while (menu(my_menu));
  return MENU_NOHOLD;
}

/******************************************************************************/

int
tst_vt220_screen(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Test Send/Receive mode (SRM)",                      tst_SRM },
      { "Test Visible/Invisible Cursor (DECTCEM)",           tst_DECTCEM },
      { "Test Erase Char (ECH)",                             tst_ECH },
      { "Test Protected-Areas (DECSCA)",                     tst_DECSCA },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  (void)the_title;
  do {
    vt_clear(2);
    title(0);
    printf("VT220 Screen-Display Tests");
    title(2);
    println("Choose test type:");
  } while (menu(my_menu));
  return MENU_NOHOLD;
}

/******************************************************************************/

int
tst_vt220_reports(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Test Device Status Report (DSR)",                   tst_vt220_device_status },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  (void)the_title;
  do {
    vt_clear(2);
    __(title(0), printf("VT220 Reports"));
    __(title(2), println("Choose test type:"));
  } while (menu(my_menu));
  return MENU_NOHOLD;
}

/******************************************************************************/

int
tst_vt220(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Test reporting functions",                          tst_vt220_reports },
      { "Test screen-display functions",                     tst_vt220_screen },
      { "Test 8-bit controls (S7C1T/S8C1T)",                 tst_S8C1T },
      { "Test Printer (MC)",                                 tst_printing },
      { "Test Soft Character Sets (DECDLD)",                 tst_softchars },
      { "Test Soft Terminal Reset (DECSTR)",                 tst_DECSTR },
      { "Test User-Defined Keys (DECUDK)",                   tst_DECUDK },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  (void)the_title;
  do {
    vt_clear(2);
    title(0);
    printf("VT220 Tests");
    title(2);
    println("Choose test type:");
  } while (menu(my_menu));
  return MENU_NOHOLD;
}
