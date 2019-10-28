/* $Id: vt320.c,v 1.62 2018/08/11 12:45:06 tom Exp $ */

/*
 * Reference:  VT330/VT340 Programmer Reference Manual (EK-VT3XX-TP-001)
 */
#include <vttest.h>
#include <ttymodes.h>
#include <draw.h>
#include <esc.h>

static void
show_Locator_Status(char *report)
{
  int pos = 0;
  int code = scanto(report, &pos, 'n');
  const char *show;

  switch (code) {
  case 53:
    show = "No locator";
    break;
  case 50:
    show = "Locator ready";
    break;
  case 58:
    show = "Locator busy";
    break;
  default:
    show = SHOW_FAILURE;
  }
  show_result("%s", show);
}

/*
 * Though some people refer to the locator controls as "vt220", it appears in
 * later terminals (documented in the vt320 manual, but introduced as in UWS).
 */
int
tst_DSR_locator(MENU_ARGS)
{
  return any_DSR(PASS_ARGS, "?53n", show_Locator_Status);
}

static void
show_ExtendedCursorPosition(char *report)
{
  int pos = 0;
  int Pl = scan_any(report, &pos, 'R');
  int Pc = scan_any(report, &pos, 'R');
  int Pp = scan_any(report, &pos, 'R');

  if (Pl != 0 && Pc != 0) {
    if (Pp != 0)
      show_result("Line %d, Column %d, Page %d", Pl, Pc, Pp);
    else
      show_result("Line %d, Column %d (Page?)", Pl, Pc);
  } else
    show_result(SHOW_FAILURE);
}

/* vt340/vt420 & up */
int
tst_DSR_cursor(MENU_ARGS)
{
  return any_DSR(PASS_ARGS, "?6n", show_ExtendedCursorPosition);
}

/******************************************************************************/

int
tst_vt320_device_status(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Test VT220 features",                               tst_vt220_device_status },
      { "Test Keyboard Status",                              tst_DSR_keyboard },
      { "Test Printer Status",                               tst_DSR_printer },
      { "Test UDK Status",                                   tst_DSR_userkeys },
      { "Test Locator Status",                               tst_DSR_locator },
      { "Test Extended Cursor-Position (DECXCPR)",           tst_DSR_cursor },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  (void)the_title;
  do {
    vt_clear(2);
    __(title(0), printf("VT320 Device Status Reports (DSR)"));
    __(title(2), println("Choose test type:"));
  } while (menu(my_menu));
  return MENU_NOHOLD;
}

/******************************************************************************/

/*
 * DECCIR returns single characters separated by semicolons.  It's not clear
 * (unless you test on a DEC terminal) from the documentation, which only cites
 * their values.  This function returns an equivalent-value, recovering from
 * the bogus implementations that return a decimal number.
 */
static int
scan_chr(const char *str, int *pos, int toc)
{
  int value = 0;
  while (str[*pos] != '\0' && str[*pos] != toc) {
    value = (value * 256) + (unsigned char) str[*pos];
    *pos += 1;
  }
  if (str[*pos] == toc)
    *pos += 1;
  return value;
}

/*
 * From Kermit 3.13 & VT220 pocket guide
 *
 * Request  CSI 1 $ w             cursor information report
 * Response DCS 1 $ u Pr; Pc; Pp; Srend; Satt; Sflag; Pgl; Pgr; Scss; Sdesig ST
 *        where   Pr is cursor row (counted from origin as 1,1)
 *                Pc is cursor column
 *                Pp is 1, video page, a constant for VT320s
 *                Srend = 40h + 8 (rev video on) + 4 (blinking on)
 *                                 + 2 (underline on) + 1 (bold on)
 *                Satt = 40h + 1  (selective erase on)
 *                Sflag = 40h + 8 (autowrap pending) + 4 (SS3 pending)
 *                                + 2 (SS2 pending) + 1 (Origin mode on)
 *                Pgl = char set in GL (0 = G0, 1 = G1, 2 = G2, 3 = G3)
 *                Pgr = char set in GR (same as for Pgl)
 *                Scss = 40h + 8 (G3 is 96 char) + 4 (G2 is 96 char)
 *                                + 2 (G1 is 96 char) + 1 (G0 is 96 char)
 *                Sdesig is string of character idents for sets G0...G3, with
 *                                no separators between set idents.
 *                If NRCs are active the set idents (all 94 byte types) are:
 *                British         A       Italian         Y
 *                Dutch           4       Norwegian/Danish ' (hex 60) or E or 6
 *                Finnish         5 or C  Portuguese      %6 or g or L
 *                French          R or f  Spanish         Z
 *                French Canadian 9 or Q  Swedish         7 or H
 *                German          K       Swiss           =
 *                Hebrew          %=
 *                (MS Kermit uses any choice when there are multiple)
 */

typedef struct {
  /* position */
  int row;
  int col;
  int page;
  /* current rendition */
  int Srend;                    /* raw rendition information */
  int reverse;
  int blinking;
  int underline;
  int bold;
  /* attributes */
  int Satt;                     /* raw attribute information */
  int selective_erase;
  /* flags */
  int Sflag;                    /* raw flag-information */
  int aw_pending;
  int ss3_pending;
  int ss2_pending;
  int origin_mode;
  /* character sets */
  int gl;
  int gr;
  int Scss;
  char cs_suffix[4][2];
  int cs_sizes[4];
  const char *cs_names[4];
} DECCIR_REPORT;

static int
parse_DECCIR(const char *input, DECCIR_REPORT * output)
{
  int valid = 1;
  int pos = 3;                  /* skip "1$u" */
  int n;

  memset(output, 0, sizeof(*output));

  /* *INDENT-EQLS* */
  output->row  = scanto(input, &pos, ';');
  output->col  = scanto(input, &pos, ';');
  output->page = scanto(input, &pos, ';');

  output->Srend = scan_chr(input, &pos, ';');
  if (output->Srend & 0x40) {
    /* *INDENT-EQLS* */
    output->reverse   = ((output->Srend & 0x08) != 0);
    output->blinking  = ((output->Srend & 0x04) != 0);
    output->underline = ((output->Srend & 0x02) != 0);
    output->bold      = ((output->Srend & 0x01) != 0);
  }

  output->Satt = scan_chr(input, &pos, ';');
  if (output->Satt & 0x40) {
    output->selective_erase = ((output->Satt & 1) != 0);
  }

  output->Sflag = scan_chr(input, &pos, ';');
  if (output->Sflag & 0x40) {
    /* *INDENT-EQLS* */
    output->aw_pending  = ((output->Sflag & 0x08) != 0);
    output->ss3_pending = ((output->Sflag & 0x04) != 0);
    output->ss2_pending = ((output->Sflag & 0x02) != 0);
    output->origin_mode = ((output->Sflag & 0x01) != 0);
  }

  output->gl = scanto(input, &pos, ';');
  output->gr = scanto(input, &pos, ';');

  output->Scss = scan_chr(input, &pos, ';');
  if (output->Scss & 0x40) {
    output->cs_sizes[3] = (output->Scss & 0x08) ? 96 : 94;
    output->cs_sizes[2] = (output->Scss & 0x04) ? 96 : 94;
    output->cs_sizes[1] = (output->Scss & 0x02) ? 96 : 94;
    output->cs_sizes[0] = (output->Scss & 0x01) ? 96 : 94;
  }

  n = 0;
  while (input[pos] != '\0') {
    strncpy(output->cs_suffix[n], input + pos, (size_t) 2);
    if (output->cs_suffix[n][0] != '%') {
      output->cs_suffix[n][1] = '\0';
    }
    output->cs_names[n++] = parse_Sdesig(input, &pos);
    if (n >= 4)
      break;
  }

  return valid;
}

static int
read_DECCIR(DECCIR_REPORT * output)
{
  char *report;
  int Ps = 1;

  do_csi("%d$w", Ps);
  report = get_reply();
  if ((report = skip_dcs(report)) != 0
      && strip_terminator(report)
      && *report == Ps + '0'
      && !strncmp(report + 1, "$u", (size_t) 2)) {
    return parse_DECCIR(report, output);
  } else {
    return 0;
  }
}

#define show_DECCIR_flag(value, mask, string) \
  if (value & mask) { value &= ~mask; show_result(string); }

static void
show_DECCIR(char *report)
{
  int row;
  int n;
  DECCIR_REPORT data;

  parse_DECCIR(report, &data);

  vt_move(5, 10);
  show_result("Cursor (%d,%d), page %d", data.row, data.col, data.page);

  vt_move(6, 10);
  if ((data.Srend & 0x4f) == data.Srend) {
    printf("Rendition:");
    if (data.Srend == 0) {
      show_result(" normal");
    } else {
      printf("%s", data.reverse ? " reverse" : "");
      printf("%s", data.blinking ? " blinking" : "");
      printf("%s", data.underline ? " underline" : "");
      printf("%s", data.bold ? " bold" : "");
    }
  } else {
    show_result(" -> unknown rendition (0x%x)", data.Srend);
  }

  vt_move(7, 10);
  if ((data.Satt & 0x4f) == data.Satt) {
    show_result("Selective erase: %s", data.selective_erase ? "ON" : "off");
  } else {
    show_result(" -> unknown attribute (0x%x)", data.Satt);
  }

  vt_move(8, 10);
  if ((data.Sflag & 0x4f) == data.Sflag) {
    printf("Flags:");
    printf("%s", data.aw_pending ? " autowrap pending" : "");
    printf("%s", data.ss3_pending ? " SS3 pending" : "");
    printf("%s", data.ss2_pending ? " SS2 pending" : "");
    printf("%s", data.origin_mode ? " origin-mode on" : "");
  } else {
    show_result(" -> unknown flag (0x%x)", data.Sflag);
  }

  vt_move(9, 10);
  show_result("Char set in GL: G%d, Char set in GR: G%d", data.gl, data.gr);

  vt_move(10, 10);
  if ((data.Scss & 0x4f) == data.Scss) {
    printf("Char set sizes:");
    for (n = 3; n >= 0; n--) {
      printf(" G%d(%d)", n, data.cs_sizes[n]);
    }
  } else {
    show_result(" -> unknown char set size (0x%x)", data.Scss);
  }

  row = 11;
  vt_move(row, 10);
  show_result("Character set idents for G0...G3: ");
  println("");
  for (n = 0; n < 4; ++n) {
    show_result("            %s\n", (data.cs_names[n]
                                     ? data.cs_names[n]
                                     : "?"));
    println("");
  }
}

/******************************************************************************/

/*
 * Request  CSI 2 $ w             tab stop report
 * Response DCS 2 $ u Pc/Pc/...Pc ST
 *        Pc are column numbers (from 1) where tab stops occur. Note the
 *        separator "/" occurs in a real VT320 but should have been ";".
 */
static void
show_DECTABSR(char *report)
{
  int pos = 3;                  /* skip "2$u" */
  int stop;
  char *buffer = (char *) malloc(strlen(report));

  *buffer = '\0';
  strcat(report, "/");  /* simplify scanning */
  while ((stop = scanto(report, &pos, '/')) != 0) {
    sprintf(buffer + strlen(buffer), " %d", stop);
  }
  println("");
  show_result("Tab stops:%s", buffer);
  free(buffer);
}

/******************************************************************************/

int
any_decrqpsr(MENU_ARGS, int Ps)
{
  char *report;
  int row;

  vt_move(1, 1);
  printf("Testing DECRQPSR: %s\n", the_title);

  set_tty_raw(TRUE);
  set_tty_echo(FALSE);

  vt_move(row = 3, 10);
  do_csi("%d$w", Ps);
  report = get_reply();
  chrprint2(report, row, 1);
  if ((report = skip_dcs(report)) != 0) {
    if (strip_terminator(report)
        && *report == Ps + '0'
        && !strncmp(report + 1, "$u", (size_t) 2)) {
      show_result("%s (valid request)", SHOW_SUCCESS);
      switch (Ps) {
      case 1:
        show_DECCIR(report);
        break;
      case 2:
        show_DECTABSR(report);
        break;
      }
    } else {
      show_result(SHOW_FAILURE);
    }
  } else {
    show_result(SHOW_FAILURE);
  }

  restore_ttymodes();
  vt_move(max_lines - 1, 1);
  return MENU_HOLD;
}

static int
tst_DECCIR(MENU_ARGS)
{
  return any_decrqpsr(PASS_ARGS, 1);
}

static int
tst_DECTABSR(MENU_ARGS)
{
  return any_decrqpsr(PASS_ARGS, 2);
}

/*
 * The restore should move the cursor to 2,1; the relative move back to the
 * test-grid demonstrates that.
 */
static void
tst_restore_cursor(const char *old_mode, int row, int col)
{
  print_str(old_mode);
  fflush(stdout);   /* ensure terminal is updated */
  if (row > 2)
    cud(row - 2);
  if (col > 1)
    cuf(col - 1);
}

static int
tst_DECRSPS_cursor(MENU_ARGS)
{
  char *old_mode;
  char *s;
  DECCIR_REPORT actual;

  vt_move(1, 1);
  printf("Testing %s\n", the_title);

  set_tty_raw(TRUE);
  set_tty_echo(FALSE);

  decrqpsr(1);
  old_mode = strdup(instr());

  if ((s = strchr(old_mode, 'u')) != 0) {
    int item;
    int row, col;
    int len;
    int j, k;
    int tries, fails;
    char temp[80];

    println("");
    println("Position/rendition:");
    *s = 't';

    for (item = 0; item < 16; ++item) {
      cup(row = item + 4, col = 10);
      sprintf(temp, "rendition[%2d]: ", item);
      len = print_str(temp);
      if (item == 0) {
        len += print_str(" none");
      } else {
        if (item & 8) {
          len += print_chr(' ');
          sgr("7");
          len += print_str("reverse");
        }
        if (item & 4) {
          len += print_chr(' ');
          sgr("5");
          len += print_str("blinking");
        }
        if (item & 2) {
          len += print_chr(' ');
          sgr("4");
          len += print_str("underline");
        }
        if (item & 1) {
          len += print_chr(' ');
          sgr("1");
          len += print_str("bold");
        }
      }
      if (read_DECCIR(&actual)) {
        tst_restore_cursor(old_mode, row, col + len);
        if (actual.Srend != (0x40 | item)) {
          printf(" (rendition?)");
        } else if (actual.row != row) {
          printf(" (row?)");
        } else if (actual.col != (col + len)) {
          printf(" (col?)");
        } else {
          printf(" (OK)");
        }
      } else {
        tst_restore_cursor(old_mode, row, col + len);
        printf(" (N/A)");
      }
    }

    cup(max_lines - 1, 1);
    restore_ttymodes();
    holdit();

    set_tty_raw(TRUE);
    set_tty_echo(FALSE);

    vt_move(1, 1);
    printf("Testing %s\n", the_title);
    ed(0);
    println("");
    println("Flags:");

    len = 42;
    vt_move(row = 4, col = 10);
    printf("%-*s", len, "Selective erase, should be blank:");
    tst_restore_cursor(old_mode, row, col + len);
    printf("XXXXXX");
    vt_move(row, col + len);
    decsel(0);

    vt_move(++row, col);
    printf("%-*s", len, "Selective erase, should not be blank:");
    vt_move(row, col + len);
    decsca(1);
    printf("XXXXXX");
    vt_move(row, col + len);
    decsel(0);

    fails = 0;
    tries = 0;
    for (j = ++row; j <= max_lines; ++j) {
      cup(j, min_cols - 1);
      for (k = 0; k < 3; k++) {
        print_chr('0' + k);
        fflush(stdout);
        if (read_DECCIR(&actual)) {
          tries++;
          if (actual.aw_pending ^ (k == 1))
            fails += 1;
        }
        if (j >= max_lines && k > 0)
          break;
      }
    }

    vt_move(row, 1);
    ed(0);
    println("");
    println("Modes:");

    vt_move(row += 2, col);
    el(2);
    if (fails) {
      printf("Autowrap-pending: failed %d of %d tries", fails, tries);
    } else {
      printf("Autowrap-pending: OK");
    }
    println("");

    fails = 1;
    vt_move(++row, col);
    print_str(ss2_output());
    if (read_DECCIR(&actual) && actual.ss2_pending) {
      print_chr(' ');
      if (read_DECCIR(&actual) && !actual.ss2_pending) {
        fails = 0;
      }
    }
    el(2);
    vt_move(row, col);
    if (fails) {
      println("SS2 pending: ERR");
    } else {
      println("SS2 pending: OK");
    }

    fails = 1;
    vt_move(++row, col);
    print_str(ss3_output());
    if (read_DECCIR(&actual) && actual.ss3_pending) {
      print_chr(' ');
      if (read_DECCIR(&actual) && !actual.ss3_pending) {
        fails = 0;
      }
    }
    el(2);
    vt_move(row, col);
    if (fails) {
      println("SS3 pending: ERR");
    } else {
      println("SS3 pending: OK");
    }

    fails = 1;
    vt_move(++row, col);
    decom(1);
    if (read_DECCIR(&actual) && actual.origin_mode) {
      fails = 0;
    }
    print_str(old_mode);  /* restore original settings */
    vt_move(row, col);
    printf("Origin mode: %s", fails ? "ERR" : "OK");

    vt_move(++row, 1);
    ed(0);
    println("");
    println("Character sets:");

    esc("+A");  /* set G3 to British */
    esc("o");   /* select that into GL */
    esc("*0");  /* set G2 to DEC special graphics */
    esc("}");   /* select that into GR */
    esc("(<");  /* set G1 to DEC supplementary */
    esc(")>");  /* set G0 to DEC technical */
    read_DECCIR(&actual);
    print_str(old_mode);

    vt_move(row += 2, col);
    printf("Current GL: %s", (actual.gl == 3) ? "OK" : "ERR");

    vt_move(++row, col);
    printf("Current GR: %s", (actual.gr == 2) ? "OK" : "ERR");

    for (j = 0; j < 4; ++j) {
      static const char *my_suffix = "<>0A";

      vt_move(++row, col);
      printf("G%d suffix: '%.2s' %s (%s)", j,
             actual.cs_suffix[j],
             (actual.cs_suffix[j][0] == my_suffix[j]) ? "OK" : "ERR",
             actual.cs_names[j]);
    }

    print_str(old_mode);  /* restore original settings */
  }

  free(old_mode);
  restore_ttymodes();
  vt_move(max_lines - 1, 1);
  ed(0);
  return MENU_HOLD;
}

static void
tabstop_ruler(const char *tabsr, int row, int col)
{
  int valid = 1;
  int n;
  int tabstops = 0;
  char *expect = malloc((size_t) min_cols + 1);

  vt_move(row, col);

  if (expect != 0) {
    const char *suffix;
    const char *s;

    for (n = 0; n < min_cols; ++n) {
      expect[n] = '-';
      if (((n + 1) % 10) == 0) {
        expect[n] = (char) ((((n + 1) / 10) % 10) + '0');
      } else if (((n + 1) % 5) == 0) {
        expect[n] = '+';
      }
    }
    expect[min_cols] = '\0';

    if (!strncmp(tabsr, "\033P", (size_t) 2)) {
      suffix = "\033\\";
      s = tabsr + 2;
    } else if ((unsigned char) *tabsr == 0x90) {
      suffix = "\234";
      s = tabsr + 1;
    } else {
      suffix = 0;
      s = 0;
    }

    if (s != 0 && !strncmp(s, "2$u", (size_t) 2)) {
      int value = 0;
      s += 2;
      while (*++s != '\0') {
        if (*s >= '0' && *s <= '9') {
          value = (value * 10) + (*s - '0');
        } else if (*s == '/') {
          if (value-- > 0 && value < min_cols) {
            if (expect[value] != '*') {
              expect[value] = '*';
              ++tabstops;
              value = 0;
            } else {
              valid = 0;
              break;
            }
          }
        } else {
          if (strcmp(s, suffix))
            valid = 0;
          break;
        }
      }
    } else {
      valid = 0;
    }
  } else {
    valid = 0;
  }

  if (valid) {
    println(expect);
    print_chr('*');
    for (n = 1; n < tabstops; ++n) {
      print_chr('\t');
      print_chr('*');
    }
  } else {
    println("Invalid:");
    chrprint2(tabsr, row, col);
  }
  free(expect);
}

static int
tst_DECRSPS_tabs(MENU_ARGS)
{
  int row, col, stop;
  char *old_tabs;
  char *new_tabs;
  char *s;

  vt_move(1, 1);
  printf("Testing %s\n", the_title);

  set_tty_raw(TRUE);
  set_tty_echo(FALSE);

  println("");
  println("Original:");
  decrqpsr(2);
  old_tabs = strdup(instr());
  tabstop_ruler(old_tabs, 4, 1);

  vt_move(row = 7, 1);
  println("Modified:");
  ++row;
  for (stop = 7; stop >= 4; --stop) {
    tbc(3);     /* clear existing tab-stops */
    for (col = 0; col < min_cols; col += stop) {
      cup(row, 1 + col);
      hts();
    }
    decrqpsr(2);
    new_tabs = instr();
    tabstop_ruler(new_tabs, row, 1);
    row += 2;
  }

  println("");
  println("Restore:");
  if ((s = strchr(old_tabs, 'u')) != 0)
    *s = 't';
  print_str(old_tabs);  /* restore original tab-stops */
  free(old_tabs);

  decrqpsr(2);
  new_tabs = instr();
  tabstop_ruler(new_tabs, row + 2, 1);

  restore_ttymodes();
  vt_move(max_lines - 1, 1);
  return MENU_HOLD;
}

/* Test Window Report - VT340, VT420 */
static int
tst_DECRQDE(MENU_ARGS)
{
  char *report;
  char chr;
  int Ph, Pw, Pml, Pmt, Pmp;
  int row, col;

  (void)the_title;
  vt_move(1, 1);
  println("Testing DECRQDE/DECRPDE Window Report");

  set_tty_raw(TRUE);
  set_tty_echo(FALSE);

  do_csi("\"v");
  report = get_reply();
  vt_move(row = 3, col = 10);
  chrprint2(report, row, col);

  if ((report = skip_csi(report)) != 0
      && sscanf(report, "%d;%d;%d;%d;%d\"%c",
                &Ph, &Pw, &Pml, &Pmt, &Pmp, &chr) == 6
      && chr == 'w') {
    vt_move(5, 10);
    show_result("lines:%d, cols:%d, left col:%d, top line:%d, page %d",
                Ph, Pw, Pml, Pmt, Pmp);
  } else {
    show_result(SHOW_FAILURE);
  }

  restore_ttymodes();
  vt_move(max_lines - 1, 1);
  return MENU_HOLD;
}

/* Test User-Preferred Supplemental Set - VT320 */
static int
tst_DECRQUPSS(MENU_ARGS)
{
  int row, col;
  char *report;
  const char *show;

  (void)the_title;
  __(vt_move(1, 1), println("Testing DECRQUPSS/DECAUPSS Window Report"));

  set_tty_raw(TRUE);
  set_tty_echo(FALSE);

  do_csi("&u");
  report = get_reply();
  vt_move(row = 3, col = 10);
  chrprint2(report, row, col);
  if ((report = skip_dcs(report)) != 0
      && strip_terminator(report)) {
    if (!strcmp(report, "0!u%5"))
      show = "DEC Supplemental Graphic";
    else if (!strcmp(report, "1!uA"))
      show = "ISO Latin-1 supplemental";
    else
      show = "unknown";
  } else {
    show = SHOW_FAILURE;
  }
  show_result("%s", show);

  restore_ttymodes();
  vt_move(max_lines - 1, 1);
  return MENU_HOLD;
}

/* Request Terminal State Report */
static int
tst_DECRQTSR(MENU_ARGS)
{
  int row, col;
  char *report;
  const char *show;

  (void)the_title;
  vt_move(1, 1);
  println("Testing Terminal State Reports (DECRQTSR/DECTSR)");

  set_tty_raw(TRUE);
  set_tty_echo(FALSE);

  do_csi("1$u");
  report = get_reply();

  vt_move(row = 3, col = 10);
  chrprint2(report, row, col);

  if ((report = skip_dcs(report)) != 0
      && strip_terminator(report)
      && !strncmp(report, "1$s", (size_t) 3)) {
    show = SHOW_SUCCESS;
  } else {
    show = SHOW_FAILURE;
  }
  show_result("%s", show);

  restore_ttymodes();
  vt_move(max_lines - 1, 1);
  return MENU_HOLD;
}

/******************************************************************************/

static int use_DECRPM;

int
set_DECRPM(int level)
{
  int result = use_DECRPM;
  use_DECRPM = level;
  return result;
}

#define DATA(name,level) { name, #name, level }

int
any_RQM(MENU_ARGS, RQM_DATA * table, int tablesize, int private)
{
  int j, row, Pa, Ps;
  char chr;
  char *report;

  vt_move(1, 1);
  printf("Testing %s\n", the_title);

  set_tty_raw(TRUE);
  set_tty_echo(FALSE);

  for (j = row = 0; j < tablesize; j++) {
    if (use_DECRPM < table[j].level)
      continue;

    if (++row >= max_lines - 3) {
      restore_ttymodes();
      cup(max_lines - 1, 1);
      holdit();
      vt_clear(2);
      vt_move(row = 2, 1);
      set_tty_raw(TRUE);
      set_tty_echo(FALSE);
    }

    do_csi((private ? "?%d$p" : "%d$p"), table[j].mode);
    report = instr();
    printf("\n     %4d: %-10s ", table[j].mode, table[j].name);
    if (LOG_ENABLED)
      fprintf(log_fp, "Testing %s\n", table[j].name);
    chrprint2(report, row + 1, 23);
    if ((report = skip_csi(report)) != 0
        && sscanf(report, (private
                           ? "?%d;%d$%c"
                           : "%d;%d$%c"),
                  &Pa, &Ps, &chr) == 3
        && Pa == table[j].mode
        && chr == 'y') {
      switch (Ps) {
      case 0:
        show_result(" unknown");
        break;
      case 1:
        show_result(" set");
        break;
      case 2:
        show_result(" reset");
        break;
      case 3:
        show_result(" permanently set");
        break;
      case 4:
        show_result(" permanently reset");
        break;
      default:
        show_result(" ?");
        break;
      }
    } else {
      show_result(SHOW_FAILURE);
    }
  }

  restore_ttymodes();
  vt_move(max_lines - 1, 1);
  return MENU_HOLD;
}

static int
tst_ISO_DECRPM(MENU_ARGS)
{
  /* *INDENT-OFF* */
  RQM_DATA ansi_modes[] = { /* this list is sorted by code, not name */
    DATA( GATM, 3 /* guarded area transfer (disabled) */),
    DATA( KAM,  3 /* keyboard action */),
    DATA( CRM,  3 /* control representation (setup) */),
    DATA( IRM,  3 /* insert/replace */),
    DATA( SRTM, 3 /* status reporting transfer (disabled) */),
    DATA( ERM,  9 /* erasure mode (non-DEC) */),
    DATA( VEM,  3 /* vertical editing (disabled) */),
    DATA( BDSM, 9 /* bi-directional support mode (non-DEC) */),
    DATA( DCSM, 9 /* device component select mode (non-DEC) */),
    DATA( HEM,  3 /* horizontal editing (disabled) */),
    DATA( PUM,  3 /* positioning unit (disabled) */),
    DATA( SRM,  3 /* send/receive */),
    DATA( FEAM, 3 /* format effector action (disabled) */),
    DATA( FETM, 3 /* format effector transfer (disabled) */),
    DATA( MATM, 3 /* multiple area transfer (disabled) */),
    DATA( TTM,  3 /* transfer termination (disabled) */),
    DATA( SATM, 3 /* selected area transfer (disabled) */),
    DATA( TSM,  3 /* tabulation stop (disabled) */),
    DATA( EBM,  3 /* editing boundary (disabled) */),
    DATA( LNM,  3 /* line feed/new line */) };
  /* *INDENT-ON* */

  return any_RQM(PASS_ARGS, ansi_modes, TABLESIZE(ansi_modes), 0);
}

static int
tst_DEC_DECRPM(MENU_ARGS)
{
  /* *INDENT-OFF* */
  RQM_DATA dec_modes[] = { /* this list is sorted by code, not name */
    DATA( DECCKM,  3 /* cursor keys */),
    DATA( DECANM,  3 /* ANSI */),
    DATA( DECCOLM, 3 /* column */),
    DATA( DECSCLM, 3 /* scrolling */),
    DATA( DECSCNM, 3 /* screen */),
    DATA( DECOM,   3 /* origin */),
    DATA( DECAWM,  3 /* autowrap */),
    DATA( DECARM,  3 /* autorepeat */),
    DATA( DECEDM,  3 /* edit */),
    DATA( DECLTM,  3 /* line transmit */),
    DATA( DECSCFDM,3 /* space compression field delimiter */),
    DATA( DECTEM,  3 /* transmission execution */),
    DATA( DECEKEM, 3 /* edit key execution */),
    DATA( DECPFF,  3 /* print form feed */),
    DATA( DECPEX,  3 /* printer extent */),
    DATA( DECTCEM, 3 /* text cursor enable */),
    DATA( DECRLM,  5 /* left-to-right */),
    DATA( DECTEK,  3 /* 4010/4014 emulation */),
    DATA( DECHEM,  5 /* Hebrew encoding */),
    DATA( DECNRCM, 3 /* national replacement character set */),
    DATA( DECGEPM, 3 /* graphics expanded print */),
    DATA( DECGPCM, 3 /* graphics print color */),
    DATA( DECGPCS, 3 /* graphics print color syntax */),
    DATA( DECGPBM, 3 /* graphics print background */),
    DATA( DECGRPM, 3 /* graphics rotated print */),
    DATA( DEC131TM,3 /* VT131 transmit */),
    DATA( DECNAKB, 5 /* Greek/N-A Keyboard Mapping */),
    DATA( DECHCCM, 3 /* horizontal cursor coupling (disabled) */),
    DATA( DECVCCM, 3 /* vertical cursor coupling */),
    DATA( DECPCCM, 3 /* page cursor coupling */),
    DATA( DECNKM,  3 /* numeric keypad */),
    DATA( DECBKM,  3 /* backarrow key */),
    DATA( DECKBUM, 3 /* keyboard usage */),
    DATA( DECLRMM, 4 /* left/right margin mode */),
    DATA( DECXRLM, 3 /* transmit rate linking */),
    DATA( DECKPM,  4 /* keyboard positioning */),
    DATA( DECNCSM, 5 /* no clearing screen on column change */),
    DATA( DECRLCM, 5 /* right-to-left copy */),
    DATA( DECCRTSM,5 /* CRT save */),
    DATA( DECARSM, 5 /* auto resize */),
    DATA( DECMCM,  5 /* modem control */),
    DATA( DECAAM,  5 /* auto answerback */),
    DATA( DECCANSM,5 /* conceal answerback */),
    DATA( DECNULM, 5 /* null */),
    DATA( DECHDPXM,5 /* half duplex */),
    DATA( DECESKM, 5 /* enable secondary keyboard language */),
    DATA( DECOSCNM,5 /* overscan */),
    DATA( DECFWM,  5 /* framed windows */),
    DATA( DECRPL,  5 /* review previous lines */),
    DATA( DECHWUM, 5 /* host wake-up mode (CRT and energy saver) */),
    DATA( DECATCUM,5 /* alternate text color underline */),
    DATA( DECATCBM,5 /* alternate text color blink */),
    DATA( DECBBSM, 5 /* bold and blink style */),
    DATA( DECECM,  5 /* erase color */),
  };
  /* *INDENT-ON* */

  return any_RQM(PASS_ARGS, dec_modes, TABLESIZE(dec_modes), 1);
}

#undef DATA

/******************************************************************************/

int
tst_DECRPM(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "ANSI Mode Report (DECRPM)",                         tst_ISO_DECRPM },
      { "DEC Mode Report (DECRPM)",                          tst_DEC_DECRPM },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  (void)the_title;
  do {
    vt_clear(2);
    __(title(0), printf("Request Mode (DECRQM)/Report Mode (DECRPM)"));
    __(title(2), println("Choose test type:"));
  } while (menu(my_menu));
  return MENU_NOHOLD;
}

/******************************************************************************/

/*
 * The VT420 manual says that a valid response begins "DCS 0 $ r",
 * however I see "DCS 1 $ r" on a real VT420, consistently --TD (1996/09/11)
 */
int
any_decrqss2(const char *msg, const char *func, const char *expected)
{
  int row, col;
  char *report;
  const char *show;
  char buffer[80];

  vt_move(1, 1);
  printf("Testing DECRQSS: %s\n", msg);

  set_tty_raw(TRUE);
  set_tty_echo(FALSE);

  decrqss(func);
  report = get_reply();

  reset_decstbm();
  reset_decslrm();

  vt_move(row = 3, col = 10);
  chrprint2(report, row, col);
  switch (parse_decrqss(report, func)) {
  case 1:
    if (expected && strcmp(expected, report)) {
      sprintf(buffer, "ok (expect '%s', actual '%s')", expected, report);
      show = buffer;
    } else {
      show = "ok (valid request)";
    }
    break;
  case 0:
    show = "invalid request";
    break;
  default:
    show = SHOW_FAILURE;
    break;
  }
  show_result("%s", show);

  restore_ttymodes();
  vt_move(max_lines - 1, 1);
  return MENU_HOLD;
}

int
any_decrqss(const char *msg, const char *func)
{
  return any_decrqss2(msg, func, (const char *) 0);
}

static int
rpt_DECSASD(MENU_ARGS)
{
  return any_decrqss(the_title, "$}");
}

static int
rpt_DECSCA(MENU_ARGS)
{
  return any_decrqss(the_title, "\"q");
}

static int
rpt_DECSCL(MENU_ARGS)
{
  return any_decrqss(the_title, "\"p");
}

static int
rpt_DECSCPP(MENU_ARGS)
{
  return any_decrqss(the_title, "$|");
}

static int
rpt_DECSLPP(MENU_ARGS)
{
  return any_decrqss(the_title, "t");
}

static int
rpt_DECSSDT(MENU_ARGS)
{
  return any_decrqss(the_title, "$~");
}

int
rpt_DECSTBM(MENU_ARGS)
{
  return any_decrqss(the_title, "r");
}

static int
rpt_SGR(MENU_ARGS)
{
  return any_decrqss(the_title, "m");
}

static int
rpt_DECTLTC(MENU_ARGS)
{
  return any_decrqss(the_title, "'s");
}

static int
rpt_DECTTC(MENU_ARGS)
{
  return any_decrqss(the_title, "|");
}

int
tst_vt320_DECRQSS(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Select active status display (DECSASD)",            rpt_DECSASD },
      { "Set character attribute (DECSCA)",                  rpt_DECSCA },
      { "Set conformance level (DECSCL)",                    rpt_DECSCL },
      { "Set columns per page (DECSCPP)",                    rpt_DECSCPP },
      { "Set lines per page (DECSLPP)",                      rpt_DECSLPP },
      { "Set status line type (DECSSDT)",                    rpt_DECSSDT },
      { "Set top and bottom margins (DECSTBM)",              rpt_DECSTBM },
      { "Select graphic rendition (SGR)",                    rpt_SGR },
      { "Set transmit termination character (DECTTC)",       rpt_DECTTC },
      { "Transmission line termination character (DECTLTC)", rpt_DECTLTC },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  (void)the_title;
  do {
    vt_clear(2);
    __(title(0), printf("VT320 Status-String Reports"));
    __(title(2), println("Choose test type:"));
  } while (menu(my_menu));
  return MENU_NOHOLD;
}

/******************************************************************************/

/*
 * The main vt100 module tests CUP, HVP, CUF, CUB, CUU, CUD
 */
int
tst_vt320_cursor(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Test Pan down (SU)",                                tst_SU },
      { "Test Pan up (SD)",                                  tst_SD},
      { "Test Vertical Cursor Coupling (DECVCCM)",           not_impl },
      { "Test Page Cursor Coupling (DECPCCM)",               not_impl },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  (void)the_title;
  do {
    vt_clear(2);
    __(title(0), printf("VT320 Cursor-Movement Tests"));
    __(title(2), println("Choose test type:"));
  } while (menu(my_menu));
  return MENU_NOHOLD;
}

/******************************************************************************/

static int
tst_vt320_report_terminal(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Restore Terminal State (DECRSTS)",                  not_impl },
      { "Terminal State Report (DECRQTS/DECTSR)",            tst_DECRQTSR },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  (void)the_title;
  do {
    vt_clear(2);
    __(title(0), printf("VT320 Terminal State Reports"));
    __(title(2), println("Choose test type:"));
  } while (menu(my_menu));
  return MENU_NOHOLD;
}

/******************************************************************************/

int
tst_vt320_report_presentation(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Cursor Information Report (DECCIR)",                tst_DECCIR },
      { "Tab Stop Report (DECTABSR)",                        tst_DECTABSR },
      { "Request Mode (DECRQM)/Report Mode (DECRPM)",        tst_DECRPM },
      { "Restore Presentation State (DECRSPS): cursor",      tst_DECRSPS_cursor },
      { "Restore Presentation State (DECRSPS): tabstops",    tst_DECRSPS_tabs },
      { "Status-String Report (DECRQSS)",                    tst_vt320_DECRQSS },
      { "",                                                  0 }
  };
  /* *INDENT-ON* */

  int old_DECRPM = set_DECRPM(3);

  (void)the_title;
  do {
    vt_clear(2);
    __(title(0), printf("VT320 Presentation State Reports"));
    __(title(2), println("Choose test type:"));
  } while (menu(my_menu));
  set_DECRPM(old_DECRPM);
  return MENU_NOHOLD;
}

/******************************************************************************/

int
tst_vt320_reports(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Test VT220 features",                               tst_vt220_reports },
      { "Test Device Status Report (DSR)",                   tst_vt320_device_status },
      { "Test Presentation State Reports",                   tst_vt320_report_presentation },
      { "Test Terminal State Reports",                       tst_vt320_report_terminal },
      { "Test User-Preferred Supplemental Set (DECAUPSS)",   tst_DECRQUPSS },
      { "Test Window Report (DECRPDE)",                      tst_DECRQDE },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  (void)the_title;
  do {
    vt_clear(2);
    __(title(0), printf("VT320 Reports"));
    __(title(2), println("Choose test type:"));
  } while (menu(my_menu));
  return MENU_NOHOLD;
}

/******************************************************************************/

/* vt340/vt420 & up */
static int
tst_DECSCPP(MENU_ARGS)
{
  static const int table[] =
  {-1, 80, 80, 132};
  size_t n;
  char temp[80];
  int last = max_lines - 4;

  (void)the_title;
  for (n = 0; n < TABLESIZE(table); ++n) {
    int width = (table[n] < 0) ? min_cols : table[n];

    vt_clear(2);
    decaln();
    decscpp(table[n]);
    vt_move(last, 1);
    ruler(last, width);
    vt_clear(0);
    sprintf(temp, "Screen should be filled (%d of %d columns)", min_cols, width);
    println(temp);
    holdit();

  }
  decscpp(-1);
  vt_move(last, 1);
  vt_clear(0);
  println("Screen is reset to original width");

  return MENU_HOLD;
}

static int
tst_DECSLPP(MENU_ARGS)
{
  static const int table[] =
  {24, 25, 36, 48, 72, 144};
  size_t n;
  char temp[80];
  int last = max_lines - 4;

  (void)the_title;
  for (n = 0; n < TABLESIZE(table); ++n) {
    int high = (table[n] < 0) ? min_cols : table[n];

    vt_clear(2);
    decaln();
    decslpp(table[n]);
    vt_move(last, 1);
    ruler(last, min_cols);
    vt_clear(0);
    sprintf(temp, "Screen should be filled (%d of %d rows)", max_lines, high);
    println(temp);
    holdit();

  }
  decslpp(max_lines);
  vt_move(last, 1);
  vt_clear(0);
  println("Screen is reset to original height");

  return MENU_HOLD;
}

static int
tst_PageFormat(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Test set columns per page (DECSCPP)",               tst_DECSCPP },
      { "Test set lines per page (DECSLPP)",                 tst_DECSLPP },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  (void)the_title;
  do {
    vt_clear(2);
    __(title(0), printf("Page Format Tests"));
    __(title(2), println("Choose test type:"));
  } while (menu(my_menu));
  return MENU_NOHOLD;
}

/******************************************************************************/

/* vt340/vt420 & up */
static int
tst_PageMovement(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Test Next Page (NP)",                               not_impl },
      { "Test Preceding Page (PP)",                          not_impl },
      { "Test Page Position Absolute (PPA)",                 not_impl },
      { "Test Page Position Backward (PPB)",                 not_impl },
      { "Test Page Position Relative (PPR)",                 not_impl },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  (void)the_title;
  do {
    vt_clear(2);
    __(title(0), printf("Page Format Tests"));
    __(title(2), println("Choose test type:"));
  } while (menu(my_menu));
  return MENU_NOHOLD;
}

/******************************************************************************/

/* vt340/vt420 & up */
int
tst_vt320_screen(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Test VT220 features",                               tst_vt220_screen },
      { "Test Status line (DECSASD/DECSSDT)",                tst_statusline },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  (void)the_title;
  do {
    vt_clear(2);
    __(title(0), printf("VT320 Screen-Display Tests"));
    __(title(2), println("Choose test type:"));
  } while (menu(my_menu));
  return MENU_NOHOLD;
}

/******************************************************************************/

int
tst_vt320(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Test VT220 features",                               tst_vt220 },
      { "Test cursor-movement",                              tst_vt320_cursor },
      { "Test page-format controls",                         tst_PageFormat },
      { "Test page-movement controls",                       tst_PageMovement },
      { "Test reporting functions",                          tst_vt320_reports },
      { "Test screen-display functions",                     tst_vt320_screen },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  (void)the_title;
  do {
    vt_clear(2);
    __(title(0), printf("VT320 Tests"));
    __(title(2), println("Choose test type:"));
  } while (menu(my_menu));
  return MENU_NOHOLD;
}
