/* $Id: nonvt100.c,v 1.66 2018/08/23 23:21:28 tom Exp $ */

/*
 * The list of non-VT320 codes was compiled using the list of non-VT320 codes
 * described in the Kermit 3.13 documentation, combined with the ISO-6429
 * (ECMA-48) spec.
 */
#include <vttest.h>
#include <ttymodes.h>
#include <draw.h>
#include <esc.h>

int
not_impl(MENU_ARGS)
{
  vt_move(1, 1);
  printf("Sorry, test not implemented:\r\n\r\n  %s", the_title);
  vt_move(max_lines - 1, 1);
  return MENU_HOLD;
}

/*
 * VT420 doesn't do this, VT510 does.
 * It is unaffected by left/right margins.
 */
int
tst_CBT(MENU_ARGS)
{
  int n;
  int count;
  int last = max_lines - 3;
  int row;
  int lft;
  int rgt;
  char temp[80];

  test_with_margins(TRUE);

  lft = get_left_margin();
  rgt = get_right_margin();

  /* ignore left/right margins unless origin mode is set */
  if (origin_mode) {
    count = ((rgt - lft) + TABWIDTH) / TABWIDTH;
  } else {
    count = (min_cols + (TABWIDTH - 1)) / TABWIDTH;
  }

  set_colors(WHITE_ON_BLUE);

  for (row = 1; row < last; ++row) {
    cup(row, min_cols);
    cbt(count + 1);
    print_chr('*');
    for (n = 1; n <= count; n++) {
      int show = (count + 1 - n);

      cup(row, min_cols);
      cbt(n);
      slowly();
      sprintf(temp, "%d", show);
      print_str(temp);
    }
  }

  set_colors(0);

  test_with_margins(FALSE);

  vt_move(last, 1);
  ruler(last, min_cols);
  vt_clear(0);
  println(the_title);
  if (origin_mode) {
    println("The tab-stops should be numbered consecutively starting at 1 in margins.");
  } else {
    println("The tab-stops should be numbered consecutively starting at 1 in screen.");
  }
  return MENU_HOLD;
}

/* Note: CHA and HPA have identical descriptions in ECMA-48 */
/* dtterm implements this (VT400 doesn't, VT510 does) */
int
tst_CHA(MENU_ARGS)
{
  int n;
  int last = max_lines - 4;
  int top;
  int lft;
  int bot;
  int rgt;
  BOX box;

  test_with_margins(TRUE);

  set_colors(WHITE_ON_BLUE);

  top = get_top_margin();
  lft = get_left_margin();
  rgt = get_right_margin();
  bot = get_bottom_margin(last);

  if (origin_mode) {
    box.top = top + (bot - top + 1) / 4;
    box.left = lft + (rgt - lft + 1) / 4 - 1;
    box.right = rgt - (rgt - lft + 1) / 4 - 1;
    box.bottom = bot - (bot - top + 1) / 4;
  } else {
    box.top = (max_lines / 4);
    box.left = (min_cols / 4);
    box.right = (min_cols * 3) / 4 - 1;
    box.bottom = (max_lines * 3) / 4;
  }

  cup(box.top, box.left);
  for (n = box.left; n <= box.right; n++) {
    cha(n);
    print_chr('*');
  }

  for (n = box.top; n <= box.bottom; n++) {
    cup(n, box.left + n);
    cha(box.right);
    print_chr('*');
    cha(box.left);
    print_chr('*');
  }

  cup(box.bottom, box.left);
  for (n = box.right; n >= box.left; --n) {
    cha(n);
    print_chr('*');
  }

  set_colors(0);

  test_with_margins(FALSE);

  vt_move(last, 1);
  vt_clear(0);
  ruler(last, min_cols);

  println(the_title);
  if (origin_mode) {
    println("There should be a box-outline made of *'s in the middle of the margins.");
  } else {
    println("There should be a box-outline made of *'s in the middle of the screen.");
  }

  return MENU_HOLD;
}

/*
 * Kermit's documentation refers to this as CHI, ECMA-48 as CHT.
 *
 * VT420 doesn't do this, VT510 does
 */
int
tst_CHT(MENU_ARGS)
{
  int n;
  int last_tab;
  int last = max_lines - 4;
  int lft;
  int rgt;

  test_with_margins(TRUE);

  set_colors(WHITE_ON_BLUE);

  lft = get_left_margin();
  rgt = get_right_margin();
  last_tab = ((rgt * 2 - lft + 1) + (TABWIDTH - 1)) / TABWIDTH;

  vt_move(1, 1);
  println("CHT with param == 1:");
  for (n = 0; n < last_tab; n++) {
    cht(1);
    printf("*");
  }

  vt_move(4, 1);
  println("CHT with param != 1:");
  for (n = 0; n < last_tab; n++) {
    cup(5, 1);
    cht(n);
    printf("+");
  }

  vt_move(7, 1);
  println("Normal tabs:");
  for (n = 0; n < last_tab; n++) {
    printf("\t*");
  }

  set_colors(0);

  test_with_margins(FALSE);

  vt_move(last, 1);
  vt_clear(0);
  ruler(last, min_cols);
  println(the_title);
  println("The lines with *'s above should look the same (they wrap once)");
  return MENU_HOLD;
}

/* VT420 doesn't do this, VT510 does */
int
tst_CNL(MENU_ARGS)
{
  int n;
  int last = max_lines - 3;

  test_with_margins(TRUE);

  set_colors(WHITE_ON_BLUE);

  vt_move(1, 1);
  printf("1."); /* this line should not be overwritten */
  for (n = 1; n <= last - 3; n++) {
    hpa(min_cols);
    cnl(n - 1); /* CNL 0 should skip to the second line */
    slowly();
    printf("%d.", n + 1);
    vpa(2);     /* subsequently, start from the second line */
  }

  set_colors(0);

  test_with_margins(FALSE);

  vt_move(last, 1);
  vt_clear(0);
  println(the_title);
  println("The lines above this should be numbered in sequence, from 1.");
  return MENU_HOLD;
}

/*
 * VT510 & up
 *
 * There's a comment in the MS-DOS Kermit 3.13 documentation that implies CPL
 * is used to replace RI (reverse-index).  ECMA-48 doesn't specify scrolling
 * regions, DEC terminals do apparently, so for CPL and CNL we'll test this.
 */
int
tst_CPL(MENU_ARGS)
{
  int i;
  int last = max_lines - 3;

  test_with_margins(TRUE);

  set_colors(WHITE_ON_BLUE);

  vt_move(max_lines, 1);
  for (i = max_lines - 1; i > 0; i--) {
    cpl(1);
    slowly();
    printf("%d.", i);
  }

  set_colors(0);

  test_with_margins(FALSE);

  vt_move(last, 1);
  vt_clear(0);
  println(the_title);
  println("The lines above this should be numbered in sequence, from 1.");
  return MENU_HOLD;
}

/*
 * VT420 doesn't do this, VT510 does.
 * VT510 ignores margins, but honors DECOM.
 */
int
tst_HPA(MENU_ARGS)
{
  int n;
  int last = max_lines - 4;
  int top;
  int lft;
  int bot;
  int rgt;
  BOX box;

  test_with_margins(TRUE);

  set_colors(WHITE_ON_BLUE);

  top = get_top_margin();
  lft = get_left_margin();
  rgt = get_right_margin();
  bot = get_bottom_margin(last);

  if (origin_mode) {
    box.top = top + (bot - top + 1) / 4;
    box.left = lft + (rgt - lft + 1) / 4 - 1;
    box.right = rgt - (rgt - lft + 1) / 4 - 1;
    box.bottom = bot - (bot - top + 1) / 4;
  } else {
    box.top = (max_lines / 4);
    box.left = (min_cols / 4);
    box.right = (min_cols * 3) / 4 - 1;
    box.bottom = (max_lines * 3) / 4;
  }

  cup(box.top, box.left);
  for (n = box.left; n <= box.right; n++) {
    hpa(n);
    print_chr('*');
  }

  for (n = box.top; n <= box.bottom; n++) {
    cup(n, box.left + n);
    hpa(box.right);
    print_chr('*');
    hpa(box.left);
    print_chr('*');
  }

  cup(box.bottom, box.left);
  for (n = box.right; n >= box.left; --n) {
    hpa(n);
    print_chr('*');
  }

  set_colors(0);

  test_with_margins(FALSE);

  vt_move(last, 1);
  vt_clear(0);
  ruler(last, min_cols);

  println(the_title);
  if (origin_mode) {
    println("There should be a box-outline made of *'s in the middle of the margins.");
  } else {
    println("There should be a box-outline made of *'s in the middle of the screen.");
  }

  return MENU_HOLD;
}

int
tst_HPR(MENU_ARGS)
{
  int n;
  int last = max_lines - 4;
  int top;
  int lft;
  int bot;
  int rgt;
  BOX box;

  test_with_margins(TRUE);

  set_colors(WHITE_ON_BLUE);

  top = get_top_margin();
  lft = get_left_margin();
  rgt = get_right_margin();
  bot = get_bottom_margin(last);

  if (origin_mode) {
    box.top = top + (bot - top + 1) / 4;
    box.left = lft + (rgt - lft + 1) / 4 - 1;
    box.right = rgt - (rgt - lft + 1) / 4 - 1;
    box.bottom = bot - (bot - top + 1) / 4;
  } else {
    box.top = (max_lines / 4);
    box.left = (min_cols / 4);
    box.right = (min_cols * 3) / 4 - 1;
    box.bottom = (max_lines * 3) / 4;
  }

  cup(box.top, box.left);
  for (n = box.left; n <= box.right; n++) {
    if (n > box.left) {
      cup(box.top, box.left);
      hpr(n - box.left);
    }
    print_chr('*');
  }

  for (n = box.top; n <= box.bottom; n++) {
    cup(n, 1);
    if (box.left > 1)
      hpr(box.left - 1);
    print_chr('*');
    hpr(box.right - box.left - 1);
    print_chr('*');
  }

  for (n = box.right; n >= box.left; --n) {
    if (n > box.left) {
      cup(box.bottom, box.left);
      hpr(n - box.left);
    }
    print_chr('*');
  }

  set_colors(0);

  test_with_margins(FALSE);

  vt_move(last, 1);
  vt_clear(0);
  ruler(last, min_cols);

  println(the_title);
  if (origin_mode) {
    println("There should be a box-outline made of *'s in the middle of the margins.");
  } else {
    println("There should be a box-outline made of *'s in the middle of the screen.");
  }

  return MENU_HOLD;
}

/*
 * Neither VT420 nor VT510.
 */
static int
tst_REP(MENU_ARGS)
{
  int n;
  int last = max_lines - 5;

  test_with_margins(TRUE);
  vt_move(1, 1);
  for (n = 1; n < last; n++) {
    if (n > 1) {
      printf(" ");
      if (n > 2)
        rep(n - 2);
    }
    printf("+");
    rep(1);     /* make that 2 +'s */
    rep(10);    /* this should be ignored, since a control sequence precedes */
    println("");
  }
  test_with_margins(FALSE);

  vt_move(last, 1);
  for (n = 1; n <= min_cols; n++)
    printf("%c", (n == last || n == last + 1) ? '+' : '*');
  vt_move(last + 1, 1);
  ruler(last + 1, min_cols);
  println(the_title);
  println("There should be a diagonal of 2 +'s down to the row of *'s above this message.");
  println("The ++ in the row of *'s is the target.  If there are 11 +'s, ECMA-48 does");
  println("not prohibit this, but treats it as undefined behavior (still nonstandard).");
  return MENU_HOLD;
}

/*
 * Test the SD (scroll-down) by forcing characters written in a diagonal into
 * a horizontal row.
 *
 * VT400 and dtterm use the (incorrect?) escape sequence (ending with 'T'
 * instead of '^'), apparently someone misread 05/14 as 05/04 or vice versa.
 */
int
tst_SD(MENU_ARGS)
{
  int n;
  int last = max_lines - 3;

  for (n = 1; n < last; n++) {
    cup(n, n);
    printf("*");
    slowly();
    sd(1);
  }
  vt_move(last + 1, 1);
  ruler(last + 1, min_cols);
  vt_clear(0);
  println(the_title);
  println("There should be a horizontal row of *'s above, just above the message.");
  return MENU_HOLD;
}

/*
 * not in VT510
 *
 * Test the SL (scroll-left) by forcing characters written in a diagonal into
 * a vertical line.
 */
static int
tst_SL(MENU_ARGS)
{
  int n;
  int last = max_lines - 3;

  for (n = 1; n < last; n++) {
    cup(n, min_cols / 2 + last - n);
    printf("*");
    slowly();
    sl(1);
  }
  vt_move(last, 1);
  ruler(last, min_cols);
  vt_clear(0);
  println(the_title);
  println("There should be a vertical column of *'s centered above.");
  return MENU_HOLD;
}

/*
 * not in VT510
 *
 * Test the SR (scroll-right) by forcing characters written in a diagonal into
 * a vertical line.
 */
static int
tst_SR(MENU_ARGS)
{
  int n;
  int last = max_lines - 3;

  for (n = 1; n < last; n++) {
    cup(n, min_cols / 2 - last + n);
    printf("*");
    slowly();
    sr(1);
  }
  vt_move(last, 1);
  ruler(last, min_cols);
  vt_clear(0);
  println(the_title);
  println("There should be a vertical column of *'s centered above.");
  return MENU_HOLD;
}

/*
 * Test the SU (scroll-up) by forcing characters written in a diagonal into
 * a horizontal row.
 */
int
tst_SU(MENU_ARGS)
{
  int n;
  int last = max_lines - 3;

  for (n = 1; n < last; n++) {
    cup(last + 1 - n, n);
    printf("*");
    slowly();
    su(1);
  }
  vt_move(last + 1, 1);
  ruler(last, min_cols);
  vt_clear(0);
  println(the_title);
  println("There should be a horizontal row of *'s above, on the top row.");
  return MENU_HOLD;
}

/******************************************************************************/

static int erm_flag;

static int
toggle_ERM(MENU_ARGS)
{
  (void)the_title;
  erm_flag = !erm_flag;
  if (erm_flag)
    sm("6");
  else
    rm("6");
  return MENU_NOHOLD;
}

/*
 * Test SPA (set-protected area).
 */
static int
tst_SPA(MENU_ARGS)
{
  BOX box;

  if (make_box_params(&box, 5, 20) == 0) {
    int pass;

    for (pass = 0; pass < 2; pass++) {
      if (pass == 0) {
        esc("V");   /* SPA */
      }
      /* make two passes so we can paint over the protected-chars in the second */
      draw_box_filled(&box, '*');
      if (pass == 0) {
        esc("W");   /* EPA */

        cup(max_lines / 2, min_cols / 2);
        ed(0);  /* after the cursor */
        ed(1);  /* before the cursor */
        ed(2);  /* the whole display */

        el(0);  /* after the cursor */
        el(1);  /* before the cursor */
        el(2);  /* the whole line */

        ech(min_cols);

        __(cup(1, 1), println(the_title));
        cup(max_lines - 4, 1);
        printf("There %s be an solid box made of *'s in the middle of the screen.\n",
               erm_flag
               ? "may"
               : "should");
        println("note: DEC terminals do not implement ERM (erase mode).");
        holdit();
      }
    }
  }

  return MENU_NOHOLD;
}

static int
tst_protected_area(MENU_ARGS)
{
  static char erm_mesg[80];
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { erm_mesg,                                            toggle_ERM },
      { "Test Protected-Areas (SPA)",                        tst_SPA },
      { "",                                                  0 },
    };
  /* *INDENT-ON* */

  do {
    vt_clear(2);
    __(title(0), printf("Protected-Areas Tests"));
    __(title(2), println("Choose test type:"));
    sprintf(erm_mesg, "%s ERM (erase mode)", STR_ENABLE(erm_flag));
  } while (menu(my_menu));
  if (erm_flag)
    toggle_ERM(PASS_ARGS);
  return MENU_NOHOLD;
}

/******************************************************************************/

/*
 * Kermit's documentation refers to this as CVA, ECMA-48 as VPA.
 * Move the cursor in the current column to the specified line.
 *
 * VT420 doesn't do this, VT510 does
 */
int
tst_VPA(MENU_ARGS)
{
  int n;
  int last = max_lines - 4;
  int top;
  int lft;
  int bot;
  int rgt;
  BOX box;

  test_with_margins(TRUE);

  set_colors(WHITE_ON_BLUE);

  top = get_top_margin();
  lft = get_left_margin();
  rgt = get_right_margin();
  bot = get_bottom_margin(last);

  if (origin_mode) {
    box.top = top + (bot - top + 1) / 4;
    box.left = lft + (rgt - lft + 1) / 4 - 1;
    box.right = rgt - (rgt - lft + 1) / 4 - 1;
    box.bottom = bot - (bot - top + 1) / 4;
  } else {
    box.top = (max_lines / 4);
    box.left = (min_cols / 4);
    box.right = (min_cols * 3) / 4 - 1;
    box.bottom = (max_lines * 3) / 4;
  }

  /*
   * BS is affected by left/right margins, but VPA is unaffected by margins.
   * Both are affected by origin mode.
   */
  if (lrmm_flag) {
    if ((box.left >= lft - 1) && (box.left < lft + 1) && (lft > 1)) {
      if (lft > 2)
        box.left = lft - 2;
      else
        box.left = lft + 1;
    }
    if ((box.right == rgt) && (rgt < min_cols))
      box.right = rgt + 1;
  }

  cup(box.top, box.left);
  for (n = box.left; n <= box.right; n++) {
    print_str("*");
    if (lrmm_flag &&
        !origin_mode &&
        (n == rgt) && (rgt < min_cols) && (n < box.right)) {
      cup(box.top, n + 1);
    }
  }
  print_str("\b");
  for (n = box.top; n <= box.bottom; n++) {
    vpa(n);
    print_str("*\b");
  }

  for (n = box.right; n >= box.left; n--) {
    if (lrmm_flag &&
        !origin_mode &&
        (((n == lft) && (lft > 1) && (n > box.left)) ||
         ((n == lft + 1) && (lft > 1) && (n > box.left)) ||
         ((n == lft - 1) && (lft > 1) && (n > box.left)) ||
         ((n == rgt + 0) && (rgt < min_cols) && (n < box.right)))) {
      cup(box.bottom, n);
      print_chr('*');
      cup(box.bottom, n);
    } else {
      print_str("\b*\b");
    }
  }
  for (n = box.top; n <= box.bottom; n++) {
    vpa(n);
    print_str("*\b");
  }

  set_colors(0);

  test_with_margins(FALSE);

  vt_move(last, 1);
  vt_clear(0);

  ruler(last, min_cols);

  println(the_title);
  if (origin_mode) {
    println("There should be a box-outline made of *'s in the middle of the margins.");
  } else {
    println("There should be a box-outline made of *'s in the middle of the screen.");
  }

  return MENU_HOLD;
}

int
tst_VPR(MENU_ARGS)
{
  int n;
  int last = max_lines - 4;
  int top;
  int lft;
  int bot;
  int rgt;
  BOX box;

  test_with_margins(TRUE);

  set_colors(WHITE_ON_BLUE);

  top = get_top_margin();
  lft = get_left_margin();
  rgt = get_right_margin();
  bot = get_bottom_margin(last);

  if (origin_mode) {
    box.top = top + (bot - top + 1) / 4;
    box.left = lft + (rgt - lft + 1) / 4 - 1;
    box.right = rgt - (rgt - lft + 1) / 4 - 1;
    box.bottom = bot - (bot - top + 1) / 4;
  } else {
    box.top = (max_lines / 4);
    box.left = (min_cols / 4);
    box.right = (min_cols * 3) / 4 - 1;
    box.bottom = (max_lines * 3) / 4;
  }

  /*
   * BS is affected by left/right margins, but VPA is unaffected by margins.
   * Both are affected by origin mode.
   */
  if (lrmm_flag) {
    if ((box.left >= lft - 1) && (box.left < lft + 1) && (lft > 1)) {
      if (lft > 2)
        box.left = lft - 2;
      else
        box.left = lft + 1;
    }
    if ((box.right == rgt) && (rgt < min_cols))
      box.right = rgt + 1;
  }

  cup(box.top, box.left);
  for (n = box.left; n <= box.right; n++) {
    print_str("*");
    if (lrmm_flag &&
        !origin_mode &&
        (n == rgt) && (rgt < min_cols) && (n < box.right)) {
      cup(box.top, n + 1);
    }
  }
  print_str("\b");
  for (n = box.top; n <= box.bottom; n++) {
    if (n > box.top) {
      cup(box.top, box.right);
      vpr(n - box.top);
    }
    print_str("*\b");
  }

  for (n = box.right; n > box.left; n--) {
    if (lrmm_flag &&
        !origin_mode &&
        (((n == lft) && (lft > 1) && (n > box.left)) ||
         ((n == lft + 1) && (lft > 1) && (n > box.left)) ||
         ((n == lft - 1) && (lft > 1) && (n > box.left)) ||
         ((n == rgt + 0) && (rgt < min_cols) && (n < box.right)))) {
      cup(box.bottom, n);
      print_chr('*');
      cup(box.bottom, n);
    } else {
      print_str("\b*\b");
    }
  }

  for (n = box.bottom; n >= box.top; --n) {
    if (n > box.top) {
      cup(box.top, box.left);
      vpr(n - box.top);
    }
    print_str("*\b");
  }

  set_colors(0);

  test_with_margins(FALSE);

  vt_move(last, 1);
  vt_clear(0);

  ruler(last, min_cols);

  println(the_title);
  if (origin_mode) {
    println("There should be a box-outline made of *'s in the middle of the margins.");
  } else {
    println("There should be a box-outline made of *'s in the middle of the screen.");
  }

  return MENU_HOLD;
}

/******************************************************************************/

/*
 * ECMA-48 mentions margins, but does not say how they might be set.  VT420
 * and up implement margins; look in vt520.c for tests of these features
 * which take into account margins.
 */
int
tst_ecma48_curs(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
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

  (void)the_title;
  do {
    vt_clear(2);
    __(title(0), printf("ISO-6429 (ECMA-48) Cursor-Movement"));
    __(title(2), println("Choose test type:"));
  } while (menu(my_menu));
  return MENU_NOHOLD;
}

int
tst_ecma48_misc(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Protected-Area Tests",                              tst_protected_area },
      { "Test Repeat (REP)",                                 tst_REP },
      { "Test Scroll-Down (SD)",                             tst_SD },
      { "Test Scroll-Left (SL)",                             tst_SL },
      { "Test Scroll-Right (SR)",                            tst_SR },
      { "Test Scroll-Up (SU)",                               tst_SU },
      { "",                                                  0 },
    };
  /* *INDENT-ON* */

  (void)the_title;
  do {
    vt_clear(2);
    __(title(0), printf("Miscellaneous ISO-6429 (ECMA-48) Tests"));
    __(title(2), println("Choose test type:"));
  } while (menu(my_menu));
  return MENU_NOHOLD;
}

/******************************************************************************/
int
tst_nonvt100(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Test of VT220 features",                            tst_vt220 },
      { "Test of VT320 features",                            tst_vt320 },
      { "Test of VT420 features",                            tst_vt420 },
      { "Test of VT520 features",                            tst_vt520 },
      { "Test ISO-6429 cursor-movement",                     tst_ecma48_curs },
      { "Test ISO-6429 colors",                              tst_colors },
      { "Test other ISO-6429 features",                      tst_ecma48_misc },
      { "Test XTERM special features",                       tst_xterm },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  (void)the_title;
  do {
    vt_clear(2);
    title(0);
    printf("Non-VT100 Tests");
    title(2);
    println("Choose test type:");
  } while (menu(my_menu));
  return MENU_NOHOLD;
}
