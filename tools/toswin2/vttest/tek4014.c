/* $Id: tek4014.c,v 1.14 2018/07/26 00:37:55 tom Exp $ */

#include <vttest.h>
#include <esc.h>
#include <ttymodes.h>

#define TEKHEIGHT 3072
#define TEKWIDTH  4096

#define FIVEBITS  037
#define HIBITS    (FIVEBITS << SHIFTHI)
#define LOBITS    (FIVEBITS << SHIFTLO)
#define SHIFTHI   7
#define SHIFTLO   2
#define TWOBITS   03

#undef GS
#undef RS
#undef US

#define GS  0x001D
#define RS  0x001E
#define US  0x001F

static char empty[1];

/*
 * Switch to/from tek4014/vt100 mode.
 */
static void
tek_enable(int flag)
{
  if (flag)
    do_csi("?38h");
  else
    esc("\003");
}

/*
 * Switch to GIN (graphics-in) mode.
 */
static void
tek_GIN(void)
{
  esc("\032");
}

/*
 * Switch back to alpha (text) mode.
 */
static void
tek_ALP(void)
{
  printf("%c", US);
}

/*
 * Select a font
 */
static void
tek_font(int code)
{
  switch (code) {
  case 1:
    esc("8");   /* large */
    break;
  case 2:
    esc("9");   /* #2 */
    break;
  case 3:
    esc(":");   /* #3 */
    break;
  default:
    esc(";");   /* small */
    break;
  }
}

/*
 * Decode 2 bytes from a mouse report as a coordinate value.
 */
static int
tek_coord(char *report, int offset)
{
  int hi = FIVEBITS & CharOf(report[offset]);
  int lo = FIVEBITS & CharOf(report[offset + 1]);
  return (hi << 5) + lo;
}

/*
 * Send coordinates for a single point.  xterm reads this info in getpoint().
 * There are several possibilities for encoding the coordinates.
 */
static void
tek_point(int pen, int y, int x)
{
  char temp[20];

  if (pen) {
    sprintf(temp, "%c\007", GS);
  } else {
    sprintf(temp, "%c", GS);
  }
  sprintf(temp + strlen(temp), "%c%c%c%c%c",
          0x20 | (((y & HIBITS) >> SHIFTHI) & FIVEBITS),
          0x60 | (((y & TWOBITS) << SHIFTLO) | (x & TWOBITS)),  /* tests/sets lo_y */
          0x60 | (((y & LOBITS) >> SHIFTLO) & FIVEBITS),  /* sets lo_y */
          0x20 | (((x & HIBITS) >> SHIFTHI) & FIVEBITS),
          0x40 | (((x & LOBITS) >> SHIFTLO) & FIVEBITS));   /* must be last */
  fprintf(stdout, "%s", temp);
  if (LOG_ENABLED) {
    fprintf(log_fp, "*Set point (%d,%d)\n", y, x);
    fputs("Send: ", log_fp);
    put_string(log_fp, temp);
    fputs("\n", log_fp);
  }
}

static void
tek_linestyle(int style)
{
  char temp[10];
  sprintf(temp, "%c", 0x60 + style);
  esc(temp);
}

static void
log_mouse_click(char *report)
{
  if (LOG_ENABLED) {
    int new_x = tek_coord(report, 1);
    int new_y = tek_coord(report, 3);
    fprintf(log_fp, "Report: ");
    if ((report[0] & 0x80) != 0
        && strchr("lmrLMR", report[0] & 0x7f) != 0) {
      fprintf(log_fp, "mouse %c", report[0] & 0x7f);
    } else {
      fprintf(log_fp, "key %d", CharOf(report[0]));
    }
    fprintf(log_fp, " (%d,%d)\n", new_y, new_x);
    fflush(log_fp);
  }
}

/*
 * Clear the display
 */
static int
tek_clear(MENU_ARGS)
{
  (void)the_title;
  tek_enable(1);
  esc("\014");
  tek_enable(0);
  return FALSE;
}

static int
tek_hello(MENU_ARGS)
{
  int n;

  tek_clear(PASS_ARGS);

  tek_enable(1);
  for (n = 0; n < 4; ++n) {
    tek_font(n);
    println("Hello world!\r");
  }
  tek_enable(0);
  return FALSE;
}

/*
 * Wait for a mouse click, printing its coordinates.  While in GIN mode, we
 * may also see keys pressed.  Exit the test when we see the same event twice
 * in a row.
 */
static int
tek_mouse_coords(MENU_ARGS)
{
  char *report;
  char status[6];

  tek_clear(PASS_ARGS);

  tek_enable(1);
  set_tty_raw(TRUE);
  set_tty_echo(FALSE);

  report = empty;
  println("Any key or mouse click twice to exit...");

  do {
    int new_x;
    int new_y;

    strncpy(status, report, (size_t) 5)[5] = 0;
    /*
     * The graphics-in mode is reset each time users send a mouse click.  So we
     * set it in the loop.
     */
    tek_GIN();
    log_mouse_click(report = instr());
    new_x = tek_coord(report, 1);
    new_y = tek_coord(report, 3);
    /*
     * If we do not start a new line after reading the mouse, we will see no
     * text.  So we do it before the rest of the report rather than after.
     */
    printf("\r\n");
    if ((report[0] & 0x80) != 0
        && strchr("lmrLMR", report[0] & 0x7f) != 0) {
      printf("mouse %c:", report[0] & 0x7f);
    } else {
      printf("key: %d", CharOf(report[0]));
    }
    printf(" (%d,%d)", new_y, new_x);
    fflush(stdout);
  } while (strcmp(report, status));

  tek_ALP();
  restore_ttymodes();
  tek_enable(0);
  return FALSE;
}

/*
 * Wait for a mouse click, drawing a line point-to-point from each click.
 * Ignore keys pressed.  Exit the test when we see the same event twice
 * in a row.
 *
 * xterm pretends the screen is 4096 by 3072 (height by width).  So the lines
 * appear in the lower-right of the screen.  Since we cannot ask xterm how
 * large the screen actually is, we do not try to (cannot) scale the lines.
 */
static int
tek_mouse_lines(MENU_ARGS)
{
  char *report;
  char status[6];
  int old_x = -1;
  int old_y = -1;
  int new_x = -1;
  int new_y = -1;

  tek_clear(PASS_ARGS);

  tek_enable(1);
  set_tty_raw(TRUE);
  set_tty_echo(FALSE);

  report = empty;
  println("Any mouse click twice to exit...");
  do {
    strncpy(status, report, (size_t) 5)[5] = 0;
    if (old_x >= 0 && old_y >= 0) {
      tek_point(0, old_y, old_x);
      tek_point(1, new_y, new_x);
      fflush(stdout);
    }
    old_x = new_x;
    old_y = new_y;
    tek_GIN();
    log_mouse_click(report = instr());
    new_x = tek_coord(report, 1);
    new_y = tek_coord(report, 3);
  } while (strcmp(report, status));

  restore_ttymodes();
  tek_enable(0);
  return FALSE;
}

/*
 * Draw a grid using a different line-type for each line, if possible.
 */
static int
tek_grid_demo(MENU_ARGS)
{
  int y, x;
  int style = 0;

  tek_clear(PASS_ARGS);
  tek_enable(1);
  for (y = 0; y <= TEKHEIGHT; y += TEKHEIGHT / 16) {
    tek_linestyle(style++ % 4);
    tek_point(0, y, 0);
    tek_point(1, y, TEKWIDTH - 1);
  }
  for (x = 0; x <= TEKWIDTH; x += TEKWIDTH / 16) {
    tek_linestyle(style++ % 4);
    tek_point(0, 0, x);
    tek_point(1, TEKHEIGHT - 1, x);
  }
  tek_ALP();    /* flush the plot */
  tek_enable(0);
  return FALSE;
}

/*
 * The common versions of xterm (X11R5, X11R6, XFree86) provide a Tektronix
 * 4014 emulation.  It can be configured out of XFree86 xterm, but that fact is
 * ignored by the cretins who quote rxvt's misleading manpage.  (The emulation
 * if unused accounts for 30kb of shared memory use).
 *
 * Other than its association with xterm, these tests do not really belong
 * in vttest, since the Tektronix is not even an ANSI terminal.
 */
int
tst_tek4014(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
    { "Exit",                                                0 },
    { "Clear screen",                                        tek_clear },
    { "'Hello World!' in each font",                         tek_hello },
    { "Get mouse-clicks, showing coordinates",               tek_mouse_coords },
    { "Get mouse-clicks, drawing lines between",             tek_mouse_lines },
    { "Draw a grid",                                         tek_grid_demo },
    { "",                                                    0 }
  };
  /* *INDENT-ON* */

  (void)the_title;
  do {
    vt_clear(2);
    __(title(0), println("XTERM/tek4014 features"));
    __(title(2), println("Choose test type:"));
  } while (menu(my_menu));
  return MENU_NOHOLD;
}
