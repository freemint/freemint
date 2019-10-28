/* $Id: color.c,v 1.39 2018/07/26 00:21:02 tom Exp $ */

#include <vttest.h>
#include <draw.h>
#include <esc.h>

#define MAX_COLORS    8

#define COLOR_BLACK   0
#define COLOR_RED     1
#define COLOR_GREEN   2
#define COLOR_YELLOW  3
#define COLOR_BLUE    4
#define COLOR_MAGENTA 5
#define COLOR_CYAN    6
#define COLOR_WHITE   7

static const char *colors[MAX_COLORS] =
{
  "black",                      /* 30, 40 */
  "red",                        /* 31, 41 */
  "green",                      /* 32, 42 */
  "yellow",                     /* 33, 43 */
  "blue",                       /* 34, 44 */
  "magenta",                    /* 35, 45 */
  "cyan",                       /* 36, 46 */
  "white"                       /* 37, 47 */
};

int do_colors = FALSE;

/*
 * Pick an unusual color combination for testing, just in case the user's
 * got the background set to something different.
 */
static void
c_sgr(const char *s)
{
  char temp[80];
  int reset = FALSE;

  (void) strcpy(temp, s);
  if (*temp == ';' || *temp == 0) {
    reset = TRUE;
  } else {
    char *t;
    for (t = temp; *t != 0; t++) {
      if (((t[0] == '0')
           && (t == temp || t[-1] == ';')
           && (t[1] == 0 || t[1] == ';'))
          || ((t[0] == ';')
              && (t[1] == ';'))) {
        reset = TRUE;
        break;
      }
    }
  }

  if (reset && do_colors) {
    sprintf(temp + strlen(temp), ";%d;%d", COLOR_YELLOW + 30, COLOR_BLUE + 40);
  }
  sgr(temp);
}

/*
 * Some terminals will reset colors with SGR-0; I've added the 39, 49 codes for
 * those that are ISO compliant.  (The black/white codes are for emulators
 * written by people who don't bother reading standards).
 */
static void
reset_all_colors(void)
{
  sgr("0;40;37;39;49");
  sgr("0");
}

static void
set_background(int bg)
{
  if (do_colors) {
    char temp[80];
    (void) sprintf(temp, "4%d", bg);
    sgr(temp);
  }
}

static void
set_color_pair(int fg, int bg)
{
  if (do_colors) {
    char temp[80];
    (void) sprintf(temp, "3%d;4%d", fg, bg);
    sgr(temp);
  }
}

static void
set_foreground(int fg)
{
  if (do_colors) {
    char temp[80];
    (void) sprintf(temp, "3%d", fg);
    sgr(temp);
  }
}

static void
set_test_colors(void)
{
  c_sgr("0");
}

static void
reset_test_colors(void)
{
  /* Now, set the background again just in case there's a glitch */
  set_foreground(COLOR_WHITE);
  set_background(COLOR_BLACK);
}

/* Graphic rendition requires special handling with color, since SGR-0
 * is supposed to reset the colors as well.
 */
static void
show_graphic_rendition(void)
{
  ed(2);
  /* *INDENT-OFF* */
  cup( 1,20); printf("Color/Graphic rendition test pattern:");
  cup( 4, 1); c_sgr("0");            printf("vanilla");
  cup( 4,40); c_sgr("0;1");          printf("bold");
  cup( 6, 6); c_sgr(";4");           printf("underline");
  cup( 6,45); c_sgr(";1");c_sgr("4");printf("bold underline");
  cup( 8, 1); c_sgr("0;5");          printf("blink");
  cup( 8,40); c_sgr("0;5;1");        printf("bold blink");
  cup(10, 6); c_sgr("0;4;5");        printf("underline blink");
  cup(10,45); c_sgr("0;1;4;5");      printf("bold underline blink");
  cup(12, 1); c_sgr("1;4;5;0;7");    printf("negative");
  cup(12,40); c_sgr("0;1;7");        printf("bold negative");
  cup(14, 6); c_sgr("0;4;7");        printf("underline negative");
  cup(14,45); c_sgr("0;1;4;7");      printf("bold underline negative");
  cup(16, 1); c_sgr("1;4;;5;7");     printf("blink negative");
  cup(16,40); c_sgr("0;1;5;7");      printf("bold blink negative");
  cup(18, 6); c_sgr("0;4;5;7");      printf("underline blink negative");
  cup(18,45); c_sgr("0;1;4;5;7");    printf("bold underline blink negative");
  cup(20, 6); c_sgr(""); set_foreground(9); printf("original foreground");
  cup(20,45); c_sgr(""); set_background(9); printf("original background");
  /* *INDENT-ON* */

  c_sgr("");    /* same as c_sgr("0") */

  decscnm(FALSE);   /* Inverse video off */
  cup(max_lines - 1, 1);
  el(0);
  printf("Dark background. ");
  holdit();

  decscnm(TRUE);  /* Inverse video */
  cup(max_lines - 1, 1);
  el(0);
  printf("Light background. ");
  holdit();

  decscnm(FALSE);
}

static void
show_line_deletions(void)
{
  int row;

  ed(2);
  cup(1, 1);
  printf("This test deletes every third line from a list, marking cursor with '*'.\n");
  printf("The foreground and background should be yellow(orange) and blue, respectively.\n");

  for (row = 5; row <= max_lines; row++) {
    cup(row, 1);
    printf("   row %3d: this is some text", row);
  }
  for (row = 7; row <= max_lines; row += 2 /* 3 - deletion */ ) {
    cup(row, 8);
    dl(1);
    putchar('*');   /* cursor should be in column 1 */
  }
  cup(3, 1);
  holdit();
}

static void
show_line_insertions(void)
{
  int row;

  ed(2);
  cup(1, 1);
  printf("This test inserts after every second line in a list, marking cursor with '*'.\n");
  printf("The foreground and background should be yellow(orange) and blue, respectively.\n");

  for (row = 5; row <= max_lines; row++) {
    cup(row, 1);
    printf("   row %3d: this is some text", row);
  }
  for (row = 7; row <= max_lines; row += 3 /* 2 + insertion */ ) {
    cup(row, 8);
    il(1);
    putchar('*');   /* cursor should be in column 1 */
  }
  cup(3, 1);
  holdit();
}

static int
show_test_pattern(MENU_ARGS)
/* generate a color test pattern */
{
  int i, j, k;

  (void)the_title;
  reset_all_colors();
  ed(2);
  cup(1, 1);
  printf("There are %d color combinations", MAX_COLORS * MAX_COLORS);

  for (k = 0; k <= 11; k += 11) {
    cup(k + 2, 1);
    printf("%dx%d matrix of foreground/background colors, bright *",
           MAX_COLORS, MAX_COLORS);

    if (k) {
      sgr("1");
      printf("on");
      sgr("0");
    } else {
      printf("off");
    }
    printf("*");

    for (i = 0; i < MAX_COLORS; i++) {
      cup(k + 3, (i + 1) * 8 + 1);
      printf("%s", colors[i]);
    }

    for (i = 0; i < MAX_COLORS; i++) {
      cup(k + i + 4, 1);
      printf("%s", colors[i]);
    }

    for (i = 0; i < MAX_COLORS; i++) {
      for (j = 0; j < MAX_COLORS; j++) {
        if (k)
          sgr("1");
        set_color_pair(j, i);
        cup(k + 4 + i, (j + 1) * 8 + 1);
        printf("Hello");
        reset_all_colors();
      }
    }
  }
  reset_all_colors();
  cup(max_lines - 1, 1);
  return MENU_HOLD;
}

/*
 * Clear around the box for simple_bce_test().
 */
static void
simple_bce_erases(BOX *box)
{
  int i;

  cup(box->top - 1, min_cols / 2);
  ed(1);        /* clear from home to cursor */
  cuf(1);
  el(0);        /* clear from cursor to end of line */

  cup(box->bottom + 1, min_cols / 2);
  ed(0);        /* clear from cursor to end */
  cub(1);
  el(1);        /* clear to beginning of line */

  for (i = box->top; i <= box->bottom; i++) {
    cup(i, box->left - 1);
    el(1);
    cup(i, box->right + 1);
    el(0);
  }
}

/*
 * "Real" color terminals support bce (background color erase).
 *
 * Set the foreground and background colors to something that's unusual.
 * Then clear the screen (the background should stick) and draw some nested
 * boxes (because that's simple). Use the ED, EL controls to clear away the
 * outer box, so we can exercise the various parameter combinations of each
 * of these.
 */
static int
simple_bce_test(MENU_ARGS)
{
  BOX box1;
  BOX box2;

  static const char *const text1[] =
  {
    "The screen background should be blue, with a box made of asterisks",
    " and this caption, in orange (non-bold yellow). ",
    " There should be no cells with the default foreground or background.",
    0
  };
  static const char *const text2[] =
  {
    "The screen background should be black, with a box made of asterisks",
    " and this caption, in white (actually gray - it is not bold). ",
    " Only the asterisk box should be in color.",
    0
  };

  (void)the_title;
  if (make_box_params(&box1, 3, 10) < 0
      || make_box_params(&box2, 7, 18) < 0)
    return MENU_NOHOLD;

  set_test_colors();
  decaln();

  draw_box_filled(&box1, 'X');
  draw_box_outline(&box2, '*');

  simple_bce_erases(&box2);

  draw_box_caption(&box2, 1, text1);

  cup(max_lines - 1, 1);
  holdit();

  reset_test_colors();

  simple_bce_erases(&box2);

  draw_box_caption(&box2, 1, text2);

  cup(max_lines - 1, 1);
  holdit();

  reset_all_colors();
  return MENU_NOHOLD;
}

/*
 * Clear around the box for fancy_bce_test().
 */
static void
fancy_bce_erases(BOX *box)
{
  int i;

  cup(box->top - 1, min_cols / 2);
  ed(1);        /* clear from home to cursor */
  cuf(1);
  el(0);        /* clear from cursor to end of line */

  cup(box->bottom + 1, min_cols / 2);
  ed(0);        /* clear from cursor to end */
  cub(1);
  el(1);        /* clear to beginning of line */

  for (i = box->top; i <= box->bottom; i++) {
    int first;
    int limit;

    cup(i, box->left - 1);
    el(1);
    cup(i, box->right + 1);
    limit = min_cols - box->right;
    first = i + 1 - box->top;
    if (first > limit)
      first = limit;
    dch(first);
    limit -= first;
    if (limit > 0)
      ech(limit);
  }
}

/*
 * Scroll the box up/down to check if the colors are being shifted in.
 */
static void
fancy_bce_shifts(BOX *box)
{
  int i;
  int limit = box->top - 1;

  decsclm(TRUE);  /* slow it down a little */
  cup(1, 1);
  for (i = 0; i < limit; ++i)
    dl(1);

  for (i = 0; i < limit; ++i)
    ri();
  decsclm(FALSE);
}

/*
 * Scroll the box up/down to check if the colors are being shifted in.  Erase
 * the colors with ECH and DCH.
 */
static int
fancy_bce_test(MENU_ARGS)
{
  BOX box1;
  BOX box2;

  static const char *const text1[] =
  {
    "The screen background should be blue, with a box made of asterisks",
    " and this caption, in orange (non-bold yellow). ",
    " There should be no cells with the default foreground or background.",
    0
  };
  static const char *const text2[] =
  {
    "The screen background should be black, with a box made of asterisks",
    " and this caption, in white (actually gray - it is not bold). ",
    " Only the asterisk box should be in color.",
    0
  };

  (void)the_title;
  if (make_box_params(&box1, 3, 10) < 0
      || make_box_params(&box2, 7, 18) < 0)
    return MENU_NOHOLD;

  set_test_colors();
  decaln();

  draw_box_filled(&box1, 'X');
  draw_box_outline(&box2, '*');

  fancy_bce_erases(&box2);
  draw_box_caption(&box2, 1, text1);
  fancy_bce_shifts(&box2);

  cup(max_lines - 1, 1);
  holdit();

  reset_test_colors();

  fancy_bce_erases(&box2);
  draw_box_caption(&box2, 1, text2);
  fancy_bce_shifts(&box2);

  cup(max_lines - 1, 1);
  holdit();

  reset_all_colors();
  return MENU_NOHOLD;
}

static int
test_color_movements(MENU_ARGS)
{
  set_test_colors();

  tst_movements(PASS_ARGS);
  reset_all_colors();
  return MENU_NOHOLD;
}

static int
test_color_screen(MENU_ARGS)
{
  set_test_colors();

  /* The rest of the test can be done nicely with the standard vt100 test
   * for insert/delete, since it doesn't modify SGR.
   */
  tst_screen(PASS_ARGS);
  reset_all_colors();
  return MENU_NOHOLD;
}

/*
 * Test the insert/delete line/character operations for color (bce) terminals
 * We'll test insert/delete line operations specially, because it is very hard
 * to see what is happening with the accordion test when it does not work.
 */
static int
test_color_insdel(MENU_ARGS)
{
  set_test_colors();

  show_line_insertions();
  show_line_deletions();

  /* The rest of the test can be done nicely with the standard vt100 test
   * for insert/delete, since it doesn't modify SGR.
   */
  tst_insdel(PASS_ARGS);
  reset_all_colors();
  return MENU_NOHOLD;
}

/*
 * Test the other ECMA-48 features with color setup.
 */
static int
test_ecma48_misc(MENU_ARGS)
{
  set_test_colors();

  tst_ecma48_misc(PASS_ARGS);
  reset_all_colors();
  return MENU_NOHOLD;
}

static int
test_bce_color(MENU_ARGS)
{
  (void)the_title;
  set_test_colors();

  do_scrolling();
  show_graphic_rendition();
  reset_all_colors();
  return MENU_NOHOLD;
}

/*
 * VT220 and higher implement the 22, 24, 25 and 27 codes.
 * VT510 implements concealed text.
 *
 * ISO 6429 specifies additional SGR codes so that one needn't use SGR 0
 * to reset everything before switching, e.g., set/clear pairs are
 * bold      1/22
 * faint     2/22
 * italics   3/23
 * underline 4/24
 * blink     5/25
 * inverse   7/27
 * concealed 8/28
 */
static int
test_iso_6429_sgr(MENU_ARGS)
{
  (void)the_title;
  set_test_colors();

  ed(2);
  /* *INDENT-OFF* */
  cup( 1,20); printf("Extended/Graphic rendition test pattern:");
  cup( 4, 1); c_sgr("0");            printf("vanilla");
  cup( 4,40); c_sgr("0;1");          printf("bold");
  cup( 6, 6); c_sgr("22;4");         printf("underline");
  cup( 6,45); c_sgr("24;1;4");       printf("bold underline");
  cup( 8, 1); c_sgr("22;24;5");      printf("blink");
  cup( 8,40); c_sgr("25;5;1");       printf("bold blink");
  cup(10, 6); c_sgr("22;4;5");       printf("underline blink");
  cup(10,45); c_sgr("24;25;1;4;5");  printf("bold underline blink");
  cup(12, 1); c_sgr("22;24;25;7");   printf("negative");
  cup(12,40); c_sgr("1");            printf("bold negative");
  cup(14, 6); c_sgr("22;4;7");       printf("underline negative");
  cup(14,45); c_sgr("1;4;7");        printf("bold underline negative");
  cup(16, 1); c_sgr("22;24;5;7");    printf("blink negative");
  cup(16,40); c_sgr("1");            printf("bold blink negative");
  cup(18, 6); c_sgr("22;4");         printf("underline blink negative");
  cup(18,45); c_sgr("1");            printf("bold underline blink negative");
  cup(20, 6); c_sgr(""); set_foreground(9); printf("original foreground");
  cup(20,45); c_sgr(""); set_background(9); printf("original background");
  cup(22, 1); c_sgr(";8");           printf("concealed");
  cup(22,40); c_sgr("8;7");          printf("concealed negative");
  /* *INDENT-ON* */

  c_sgr("");    /* same as c_sgr("0") */
  printf(" <- concealed text");

  decscnm(FALSE);   /* Inverse video off */
  cup(max_lines - 1, 1);
  el(0);
  printf("Dark background. ");
  holdit();

  decscnm(TRUE);  /* Inverse video */
  cup(max_lines - 1, 1);
  el(0);
  printf("Light background. ");
  holdit();

  decscnm(FALSE);
  cup(max_lines - 1, 1);
  el(0);
  printf("Dark background. ");
  holdit();

  reset_all_colors();
  return MENU_NOHOLD;
}

/*
 */
static int
test_SGR_0(MENU_ARGS)
{
  vt_move(1, 1);
  println(the_title);
  println("");
  println("ECMA-48 states that SGR 0 \"cancels the effect of any preceding occurrence");
  println("of SGR in the data stream regardless of the setting of the graphic rendition");
  println("combination mode (GRCM)\".");
  println("");
  println("");

  reset_all_colors();
  printf("You should see only black:");
  sgr("30;40");
  printf("SGR 30 and SGR 40 don't work");
  reset_all_colors();
  println(":up to here");

  reset_all_colors();
  printf("You should see only white:");
  sgr("37;47");
  printf("SGR 37 and SGR 47 don't work");
  reset_all_colors();
  println(":up to here");

  reset_all_colors();
  printf("You should see text here: ");
  sgr("30;40");
  sgr("0");
  printf("SGR 0 reset works (explicit 0)");
  println("");

  reset_all_colors();
  printf("................and here: ");
  sgr("37;47");
  sgr("");
  printf("SGR 0 reset works (default param)");
  println("");

  reset_all_colors();
  holdit();
  return MENU_NOHOLD;
}

/*
 * Allow user to test the same screens w/o colors.
 */
int
toggle_color_mode(MENU_ARGS)
{
  (void)the_title;
  do_colors = !do_colors;
  return MENU_NOHOLD;
}

/*
 * VT100s of course never did colors, ANSI or otherwise.  This test is for
 * xterm.
 */
static int
test_vt100_colors(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU const colormenu[] = {
    { "Exit",                                                0 },
    { "Test of cursor movements",                            test_color_movements },
    { "Test of screen features",                             test_color_screen },
    { "Test Insert/Delete Char/Line",                        test_color_insdel, },
    { "", 0 }
  };
  /* *INDENT-ON* */

  int save_colors = do_colors;

  (void)the_title;
  do_colors = TRUE;

  do {
    vt_clear(2);
    __(title(0), println("Test VT102-style features with BCE"));
    __(title(2), println("Choose test type:"));
  } while (menu(colormenu));

  do_colors = save_colors;

  return MENU_NOHOLD;
}

/*
 * For terminals that support ANSI/ISO colors, work through a graduated
 * set of tests that first display colors (if the terminal does indeed
 * support them), then exercise the associated reset, clear operations.
 */
int
tst_colors(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU const colormenu[] = {
    { "Exit",                                                0 },
    { txt_override_color,                                    toggle_color_mode, },
    { "Display color test-pattern",                          show_test_pattern, },
    { "Test SGR-0 color reset",                              test_SGR_0, },
    { "Test BCE-style clear line/display (ED, EL)",          simple_bce_test, },
    { "Test BCE-style clear line/display (ECH, Indexing)",   fancy_bce_test, },
    { "Test of VT102-style features with BCE",               test_vt100_colors, },
    { "Test other ISO-6429 features with BCE",               test_ecma48_misc },
    { "Test screen features with BCE",                       test_bce_color, },
    { "Test screen features with ISO 6429 SGR 22-27 codes",  test_iso_6429_sgr, },
    { "", 0 }
  };
  /* *INDENT-ON* */

  int save_colors = do_colors;

  (void)the_title;
  do_colors = TRUE;

  do {
    vt_clear(2);
    sprintf(txt_override_color, "%s color-switching", STR_ENABLE(do_colors));
    __(title(0), println("ISO 6429 colors"));
    __(title(2), println("Choose test type:"));
  } while (menu(colormenu));

  do_colors = save_colors;

  return MENU_NOHOLD;
}
