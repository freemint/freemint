/* $Id: esc.c,v 1.90 2018/07/25 14:03:26 tom Exp $ */

#include <vttest.h>
#include <esc.h>

/* This was needed for Solaris 2.5, whose standard I/O was broken */
#define FLUSH fflush(stdout)

static int soft_scroll;

/******************************************************************************/

static int pending_decstbm;
static int pending_decslrm;

static const char csi_7[] =
{ESC, '[', 0};

static const unsigned char csi_8[] =
{0x9b, 0};

const char *
csi_input(void)
{
  return input_8bits ? (const char *) csi_8 : csi_7;
}

const char *
csi_output(void)
{
  return output_8bits ? (const char *) csi_8 : csi_7;
}

/******************************************************************************/

static const char dcs_7[] =
{ESC, 'P', 0};

static const unsigned char dcs_8[] =
{0x90, 0};

const char *
dcs_input(void)
{
  return input_8bits ? (const char *) dcs_8 : dcs_7;
}

const char *
dcs_output(void)
{
  return output_8bits ? (const char *) dcs_8 : dcs_7;
}

/******************************************************************************/

static const char osc_7[] =
{ESC, ']', 0};
static const unsigned char osc_8[] =
{0x9d, 0};

const char *
osc_input(void)
{
  return input_8bits ? (const char *) osc_8 : osc_7;
}

const char *
osc_output(void)
{
  return output_8bits ? (const char *) osc_8 : osc_7;
}

/******************************************************************************/

static char ss2_7[] =
{ESC, 'N', 0};
static unsigned char ss2_8[] =
{0x8e, 0};

char *
ss2_input(void)
{
  return input_8bits ? (char *) ss2_8 : ss2_7;
}

char *
ss2_output(void)
{
  return output_8bits ? (char *) ss2_8 : ss2_7;
}

/******************************************************************************/

static char ss3_7[] =
{ESC, 'O', 0};
static unsigned char ss3_8[] =
{0x8f, 0};

char *
ss3_input(void)
{
  return input_8bits ? (char *) ss3_8 : ss3_7;
}

char *
ss3_output(void)
{
  return output_8bits ? (char *) ss3_8 : ss3_7;
}

/******************************************************************************/

static char st_7[] =
{ESC, '\\', 0};
static unsigned char st_8[] =
{0x9c, 0};

char *
st_input(void)
{
  return input_8bits ? (char *) st_8 : st_7;
}

char *
st_output(void)
{
  return output_8bits ? (char *) st_8 : st_7;
}

/******************************************************************************/

/*
 * The actual number of nulls for padding is an estimate; it's working at
 * 9600bd.
 */
void
padding(int msecs)
{
  if (use_padding) {
    int count = (3 * msecs * tty_speed + DEFAULT_SPEED - 1) / DEFAULT_SPEED;
    while (count-- > 0)
      putchar(0);
  }
}

void
extra_padding(int msecs)
{
  if (use_padding)
    padding(soft_scroll ? (msecs * 4) : msecs);
}

int
println(const char *s)
{
  printf("%s\r\n", s);
  if (LOG_ENABLED) {
    fprintf(log_fp, "Text: %s\n", s);
  }
  return 1;
}

void
put_char(FILE *fp, int c)
{
  if (fp == stdout)
    putchar(c);
  else {
    c &= 0xff;
    if (c <= ' ' || c >= '\177')
      fprintf(fp, "<%d> ", c);
    else
      fprintf(fp, "%c ", c);
  }
}

void
put_string(FILE *fp, const char *s)
{
  while (*s != '\0')
    put_char(fp, (int) *s++);
}

static void
va_out(FILE *fp, va_list ap, const char *fmt)
{
  char temp[10];

  while (*fmt != '\0') {
    if (*fmt == '%') {
      switch (*++fmt) {
      case 'c':
        put_char(fp, va_arg(ap, int));
        break;
      case 'd':
        sprintf(temp, "%d", va_arg(ap, int));
        put_string(fp, temp);
        break;
      case 'u':
        sprintf(temp, "%u", va_arg(ap, unsigned));
        put_string(fp, temp);
        break;
      case 's':
        put_string(fp, va_arg(ap, char *));
        break;
      }
    } else {
      put_char(fp, (int) *fmt);
    }
    fmt++;
  }
}

int
tprintf(const char *fmt,...)
{
  va_list ap;
  va_start(ap, fmt);
  va_out(stdout, ap, fmt);
  va_end(ap);
  FLUSH;

  if (LOG_ENABLED) {
    fputs("Text: ", log_fp);
    va_start(ap, fmt);
    va_out(log_fp, ap, fmt);
    va_end(ap);
    fputs("\n", log_fp);
  }
  return 1;
}

/* CSI xxx */
void
do_csi(const char *fmt,...)
{
  va_list ap;
  va_start(ap, fmt);
  fputs(csi_output(), stdout);
  va_out(stdout, ap, fmt);
  va_end(ap);
  FLUSH;

  if (LOG_ENABLED) {
    fputs("Send: ", log_fp);
    put_string(log_fp, csi_output());
    va_start(ap, fmt);
    va_out(log_fp, ap, fmt);
    va_end(ap);
    fputs("\n", log_fp);
  }
}

/* DCS xxx ST */
void
do_dcs(const char *fmt,...)
{
  va_list ap;
  va_start(ap, fmt);
  fputs(dcs_output(), stdout);
  va_out(stdout, ap, fmt);
  va_end(ap);
  fputs(st_output(), stdout);
  FLUSH;

  if (LOG_ENABLED) {
    va_start(ap, fmt);
    fputs("Send: ", log_fp);
    put_string(log_fp, dcs_output());
    va_out(log_fp, ap, fmt);
    va_end(ap);
    put_string(log_fp, st_output());
    fputs("\n", log_fp);
  }
}

/* DCS xxx ST */
void
do_osc(const char *fmt,...)
{
  va_list ap;
  va_start(ap, fmt);
  fputs(osc_output(), stdout);
  va_out(stdout, ap, fmt);
  va_end(ap);
  fputs(st_output(), stdout);
  FLUSH;

  if (LOG_ENABLED) {
    va_start(ap, fmt);
    fputs("Send: ", log_fp);
    put_string(log_fp, osc_output());
    va_out(log_fp, ap, fmt);
    va_end(ap);
    put_string(log_fp, st_output());
    fputs("\n", log_fp);
  }
}

int
print_chr(int c)
{
  printf("%c", c);

  if (LOG_ENABLED) {
    fprintf(log_fp, "Send: ");
    put_char(log_fp, c);
    fputs("\n", log_fp);
  }
  return 1;
}

int
print_str(const char *s)
{
  int result = (int) strlen(s);
  printf("%s", s);

  if (LOG_ENABLED) {
    fprintf(log_fp, "Send: ");
    while (*s) {
      put_char(log_fp, *s++);
    }
    fputs("\n", log_fp);
  }
  return result;
}

void
esc(const char *s)
{
  printf("%c%s", ESC, s);

  if (LOG_ENABLED) {
    fprintf(log_fp, "Send: ");
    put_char(log_fp, ESC);
    put_string(log_fp, s);
    fputs("\n", log_fp);
  }
}

void
brc(int pn, int c)
{
  do_csi("%d%c", pn, c);
}

void
brc2(int pn1, int pn2, int c)
{
  do_csi("%d;%d%c", pn1, pn2, c);
}

void
brc3(int pn1, int pn2, int pn3, int c)
{
  do_csi("%d;%d;%d%c", pn1, pn2, pn3, c);
}

/******************************************************************************/

void
cbt(int pn)                     /* Cursor Back Tab */
{
  brc(pn, 'Z');
}

void
cha(int pn)                     /* Cursor Character Absolute */
{
  brc(pn, 'G');
}

void
cht(int pn)                     /* Cursor Forward Tabulation */
{
  brc(pn, 'I');
}

void
cnl(int pn)                     /* Cursor Next Line */
{
  brc(pn, 'E');
}

void
cpl(int pn)                     /* Cursor Previous Line */
{
  brc(pn, 'F');
}

void
cub(int pn)                     /* Cursor Backward */
{
  brc(pn, 'D');
  padding(2);
}

void
cud(int pn)                     /* Cursor Down */
{
  brc(pn, 'B');
  extra_padding(2);
}

void
cuf(int pn)                     /* Cursor Forward */
{
  brc(pn, 'C');
  padding(2);
}

int
cup(int pn1, int pn2)           /* Cursor Position */
{
  brc2(pn1, pn2, 'H');
  padding(5);   /* 10 for vt220 */
  return 1;     /* used for indenting */
}

void
cuu(int pn)                     /* Cursor Up */
{
  brc(pn, 'A');
  extra_padding(2);
}

void
da(void)                        /* Device Attributes */
{
  brc(0, 'c');
}

void
decaln(void)                    /* Screen Alignment Display */
{
  esc("#8");
}

void
decarm(int flag)                /* DECARM autorepeat */
{
  if (flag)
    sm("?8");   /* autorepeat */
  else
    rm("?8");   /* no autorepeat */
}

void
decawm(int flag)                /* DECAWM autowrap */
{
  if (flag)
    sm("?7");   /* autowrap */
  else
    rm("?7");   /* no autowrap */
}

void
decbi(void)                     /* VT400: Back Index */
{
  esc("6");
  padding(40);
}

void
decbkm(int flag)                /* VT400: Backarrow key */
{
  if (flag)
    sm("?67");  /* backspace */
  else
    rm("?67");  /* delete */
}

void
decncsm(int flag)               /* VT500: DECNCSM no clear on DECCOLM */
{
  if (flag)
    sm("?95");  /* no clear */
  else
    rm("?95");  /* clear */
}

void
deccara(int top, int left, int bottom, int right, int attr)
{
  do_csi("%d;%d;%d;%d;%d$r", top, left, bottom, right, attr);
}

void
decckm(int flag)                /* DECCKM Cursor Keys */
{
  if (flag)
    sm("?1");   /* application */
  else
    rm("?1");   /* normal */
}

void
deccolm(int flag)               /* DECCOLM 80/132 Columns */
{
  if (flag)
    sm("?3");   /* 132 columns */
  else
    rm("?3");   /* 80 columns */
}

void
deccra(int Pts, int Pl, int Pbs, int Prs, int Pps, int Ptd, int Pld, int Ppd)
{
  do_csi("%d;%d;%d;%d;%d;%d;%d;%d;$v",
         Pts,   /* top-line border */
         Pl,    /* left-line border */
         Pbs,   /* bottom-line border */
         Prs,   /* right-line border */
         Pps,   /* source page number */
         Ptd,   /* destination top-line border */
         Pld,   /* destination left-line border */
         Ppd);  /* destination page number */
}

int
decdc(int pn)                   /* VT400 Delete Column */
{
  do_csi("%d'~", pn);
  padding(10 * pn);
  return 1;
}

void
decefr(int top, int left, int bottom, int right)  /* DECterm Enable filter rectangle */
{
  do_csi("%d;%d;%d;%d'w", top, left, bottom, right);
}

void
decelr(int all_or_one, int pixels_or_cells)   /* DECterm Enable Locator Reporting */
{
  do_csi("%d;%d'z", all_or_one, pixels_or_cells);
}

void
decera(int top, int left, int bottom, int right)  /* VT400 Erase Rectangular area */
{
  do_csi("%d;%d;%d;%d$z", top, left, bottom, right);
}

void
decdhl(int lower)               /* Double Height Line (also double width) */
{
  if (lower)
    esc("#4");
  else
    esc("#3");
}

void
decdwl(void)                    /* Double Wide Line */
{
  esc("#6");
}

void
decfi(void)                     /* VT400: Forward Index */
{
  esc("9");
  padding(40);
}

void
decfra(int c, int top, int left, int bottom, int right)   /* VT400 Fill Rectangular area */
{
  do_csi("%d;%d;%d;%d;%d$x", c, top, left, bottom, right);
}

void
decid(void)                     /* required for VT52, not recommended above VT100 */
{
  esc("Z");     /* Identify     */
}

int
decic(int pn)                   /* VT400 Insert Column */
{
  do_csi("%d'}", pn);
  padding(10 * pn);
  return 1;
}

void
deckbum(int flag)               /* VT400: Keyboard Usage */
{
  if (flag)
    sm("?68");  /* data processing */
  else
    rm("?68");  /* typewriter */
}

void
deckpam(void)                   /* Keypad Application Mode */
{
  esc("=");
}

void
deckpm(int flag)                /* VT400: Keyboard Position */
{
  if (flag)
    sm("?81");  /* position reports */
  else
    rm("?81");  /* character codes */
}

void
deckpnm(void)                   /* Keypad Numeric Mode */
{
  esc(">");
}

void
decll(const char *ps)           /* Load LEDs */
{
  do_csi("%sq", ps);
}

void
decnkm(int flag)                /* VT400: Numeric Keypad */
{
  if (flag)
    sm("?66");  /* application */
  else
    rm("?66");  /* numeric */
}

void
decom(int flag)                 /* DECOM Origin */
{
  if (flag)
    sm("?6");   /* relative */
  else
    rm("?6");   /* absolute */
}

void
decpex(int flag)                /* VT220: printer extent mode */
{
  if (flag)
    sm("?19");  /* full screen (page) */
  else
    rm("?19");  /* scrolling region */
}

void
decpff(int flag)                /* VT220: print form feed mode */
{
  if (flag)
    sm("?18");  /* form feed */
  else
    rm("?18");  /* no form feed */
}

void
decnrcm(int flag)               /* VT220: National replacement character set */
{
  if (flag)
    sm("?42");  /* national */
  else
    rm("?42");  /* multinational */
}

void
decrara(int top, int left, int bottom, int right, int attr)
{
  do_csi("%d;%d;%d;%d;%d$t", top, left, bottom, right, attr);
}

void
decrc(void)                     /* Restore Cursor */
{
  esc("8");
}

void
decreqtparm(int pn)             /* Request Terminal Parameters */
{
  brc(pn, 'x');
}

void
decrqlp(int mode)               /* DECterm Request Locator Position */
{
  do_csi("%d'|", mode);
}

void
decrqpsr(int mode)              /* Request presentation report */
{
  do_csi("%d$w", mode);
}

void
decrqss(const char *pn)         /* VT200 Request Status-String */
{
  do_dcs("$q%s", pn);
}

void
decsace(int flag)               /* VT400 Select attribute change extent */
{
  do_csi("%d*x", flag ? 2 : 0);
}

void
decsasd(int pn)                 /* VT200 Select active status display */
{
  do_csi("%d$}", pn);
}

void
decsc(void)                     /* Save Cursor */
{
  esc("7");
}

void
decsca(int pn1)                 /* VT200 select character attribute (protect) */
{
  do_csi("%d\"q", pn1);
}

void
decsclm(int flag)               /* Scrolling mode (smooth/jump) */
{
  if (flag)
    sm("?4");   /* smooth scrolling */
  else
    rm("?4");   /* jump-scrolling scrolling */
  soft_scroll = flag;
}

void
decscnm(int flag)               /* Screen mode (inverse-video) */
{
  if (flag)
    sm("?5");   /* inverse video */
  else
    rm("?5");   /* normal video */
  padding(200);
}

void
decsed(int pn1)                 /* VT200 selective erase in display */
{
  do_csi("?%dJ", pn1);
}

void
decsel(int pn1)                 /* VT200 selective erase in line */
{
  do_csi("?%dK", pn1);
}

void
decsera(int top, int left, int bottom, int right)   /* VT400 Selective erase rectangular area */
{
  do_csi("%d;%d;%d;%d${", top, left, bottom, right);
}

void
decsle(int mode)                /* DECterm Select Locator Events */
{
  do_csi("%d'{", mode);
}

void                            /* VT300 Set columns per page */
decscpp(int cols)
{
  if (cols >= 0) {
    do_csi("%d$|", cols);
  } else {
    do_csi("$|");
  }
}

void                            /* VT300 Set lines per page */
decslpp(int rows)
{
  /*
   * DEC defines 24, 25, 36, 48, 72 and 144.
   * XTerm uses codes up to 24 for window operations,
   * and 24 and up for this feature.
   */
  do_csi("%dt", rows);
}

void
decsnls(int pn)                 /* VT400 Select number of lines per screen */
{
  do_csi("%d*|", pn);
}

void
decssdt(int pn)                 /* VT200 Select status line type */
{
  do_csi("%d$~", pn);
}

void
decstbm(int pn1, int pn2)       /* Set Top and Bottom Margins */
{
  if (pn1 || pn2) {
    brc2(pn1, pn2, 'r');
    pending_decstbm = 1;
  } else {
    esc("[r");
    pending_decstbm = 0;
  }
}

void
np(void)                        /* NP - next page */
{
  do_csi("U");
}

void
pp(void)                        /* PP - previous page */
{
  do_csi("V");
}

void
ppa(int n)                      /* PPA - Page Position Absolute */
{
  do_csi("%d P", n);
}

void
ppb(int n)                      /* PPB - Page Position Backward */
{
  do_csi("%d R", n);
}

void
ppr(int n)                      /* PPR - Page Position Relative */
{
  do_csi("%d Q", n);
}

/*
 * Reset top/bottom margins, but only if we set them to non-default.
 */
void
reset_decstbm(void)
{
  if (pending_decstbm) {
    decstbm(0, 0);
  }
}

void
decslrm(int pn1, int pn2)       /* Set Left and Right Margins */
{
  if (pn1 || pn2) {
    brc2(pn1, pn2, 's');
    pending_decslrm = 1;
  } else {
    esc("[s");
    pending_decslrm = 0;
  }
}

/*
 * Reset left/right margins, but only if we set them to non-default.
 */
void
reset_decslrm(void)
{
  if (pending_decslrm) {
    decslrm(0, 0);
  }
}

void
decstr(void)                    /* VT200 Soft terminal reset */
{
  do_csi("!p");
}

void
decswl(void)                    /* Single Width Line */
{
  esc("#5");
}

void
dectst(int pn)                  /* Invoke Confidence Test */
{
  brc2(2, pn, 'y');
  fflush(stdout);
}

void
dsr(int pn)                     /* Device Status Report */
{
  brc(pn, 'n');
}

void
ed(int pn)                      /* Erase in Display */
{
  brc(pn, 'J');
  padding(50);
}

void
el(int pn)                      /* Erase in Line */
{
  brc(pn, 'K');
  padding(3);   /* 4 for vt400 */
}

void
ech(int pn)                     /* Erase character(s) */
{
  brc(pn, 'X');
}

void
hpa(int pn)                     /* HPA - Horizontal Position Absolute */
{
  brc(pn, '`');
}

void
hpr(int pn)                     /* HPR - Horizontal Position Relative */
{
  brc(pn, 'a');
}

void
hts(void)                       /* Horizontal Tabulation Set */
{
  esc("H");
}

void
hvp(int pn1, int pn2)           /* Horizontal and Vertical Position */
{
  brc2(pn1, pn2, 'f');
}

void
ind(void)                       /* Index */
{
  esc("D");
  padding(20);  /* vt220 */
}

/* The functions beginning "mc_" are variations of Media Copy (MC) */

void
mc_autoprint(int flag)          /* VT220: auto print mode */
{
  do_csi("?%di", flag ? 5 : 4);
}

void
mc_printer_controller(int flag) /* VT220: printer controller mode */
{
  do_csi("%di", flag ? 5 : 4);
}

void
mc_print_page(void)             /* VT220: print page */
{
  do_csi("i");
}

void
mc_print_composed(void)         /* VT300: print composed main display */
{
  do_csi("?10i");
}

void
mc_print_all_pages(void)        /* VT300: print composed all pages */
{
  do_csi("?11i");
}

void
mc_print_cursor_line(void)      /* VT220: print cursor line */
{
  do_csi("?1i");
}

void
mc_printer_start(int flag)      /* VT300: start/stop printer-to-host session */
{
  do_csi("?%di", flag ? 9 : 8);
}

void
mc_printer_assign(int flag)     /* VT300: assign/release printer to active session */
{
  do_csi("?%di", flag ? 18 : 19);
}

void
nel(void)                       /* Next Line */
{
  esc("E");
}

void
rep(int pn)                     /* Repeat */
{
  do_csi("%db", pn);
}

void
ri(void)                        /* Reverse Index */
{
  esc("M");
  extra_padding(5);   /* 14 on vt220 */
}

void
ris(void)                       /*  Reset to Initial State */
{
  esc("c");
  fflush(stdout);
}

void
rm(const char *ps)              /* Reset Mode */
{
  do_csi("%sl", ps);
}

void
s8c1t(int flag)                 /* Tell terminal to respond with 7-bit or 8-bit controls */
{
  if ((input_8bits = flag) != FALSE)
    esc(" G");  /* select 8-bit controls */
  else
    esc(" F");  /* select 7-bit controls */
  fflush(stdout);
  zleep(300);
}

/*
 * If g is zero,
 *    designate G0 as character set c 
 *    designate G1 as character set B (ASCII)
 *    shift-in (select G0 into GL).
 * If g is nonzero
 *    designate G0 as character set B (ASCII)
 *    designate G1 as character set c 
 *    shift-out (select G1 into GL).
 * See also scs_normal() and scs_graphics().
 */
void
scs(int g, int c)               /* Select character Set */
{
  char temp[10];

  sprintf(temp, "%c%c", g ? ')' : '(', c);
  esc(temp);

  sprintf(temp, "%c%c", g ? '(' : ')', 'B');
  esc(temp);

  print_chr(g ? SO : SI);
  padding(4);
}

void
sd(int pn)                      /* Scroll Down */
{
  brc(pn, 'T');
}

void
sgr(const char *ps)             /* Select Graphic Rendition */
{
  do_csi("%sm", ps);
  padding(2);
}

void
sl(int pn)                      /* Scroll Left */
{
  do_csi("%d @", pn);
}

void
sm(const char *ps)              /* Set Mode */
{
  do_csi("%sh", ps);
}

void
sr(int pn)                      /* Scroll Right */
{
  do_csi("%d A", pn);
}

void
srm(int flag)                   /* VT400: Send/Receive mode */
{
  if (flag)
    sm("12");   /* local echo off */
  else
    rm("12");   /* local echo on */
}

void
su(int pn)                      /* Scroll Up */
{
  brc(pn, 'S');
  extra_padding(5);
}

void
tbc(int pn)                     /* Tabulation Clear */
{
  brc(pn, 'g');
}

void
dch(int pn)                     /* Delete character */
{
  brc(pn, 'P');
}

void
ich(int pn)                     /* Insert character -- not in VT102 */
{
  brc(pn, '@');
}

void
dl(int pn)                      /* Delete line */
{
  brc(pn, 'M');
}

void
il(int pn)                      /* Insert line */
{
  brc(pn, 'L');
}

void
vpa(int pn)                     /* Vertical Position Absolute */
{
  brc(pn, 'd');
}

void
vpr(int pn)                     /* Vertical Position Relative */
{
  brc(pn, 'e');
}

void
vt52cub1(void)                  /* cursor left */
{
  esc("D");
  padding(5);
}

void
vt52cud1(void)                  /* cursor down */
{
  esc("B");
  padding(5);
}

void
vt52cuf1(void)                  /* cursor right */
{
  esc("C");
  padding(5);
}

void
vt52cup(int l, int c)           /* direct cursor address */
{
  char temp[10];
  sprintf(temp, "Y%c%c", l + 31, c + 31);
  esc(temp);
  padding(5);
}

void
vt52cuu1(void)                  /* cursor up */
{
  esc("A");
  padding(5);
}

void
vt52ed(void)                    /* erase to end of screen */
{
  esc("J");
  padding(5);
}

void
vt52el(void)                    /* erase to end of line */
{
  esc("K");
  padding(5);
}

void
vt52home(void)                  /* cursor to home */
{
  esc("H");
  padding(5);
}

void
vt52ri(void)                    /* reverse line feed */
{
  esc("I");
  padding(5);
}
