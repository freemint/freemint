/* $Id: charsets.c,v 1.63 2018/09/01 00:20:14 tom Exp $ */

/*
 * Test character-sets (e.g., SCS control, DECNRCM mode)
 */
#include <vttest.h>
#include <esc.h>

/* the values, where specified, correspond to the keyboard-language codes */
typedef enum {
  ASCII = 1,
  British = 2,
  Flemish = 3,
  French_Canadian = 4,
  Danish = 5,
  Finnish = 6,
  German = 7,
  Dutch = 8,
  Italian = 9,
  Swiss_French = 10,
  Swiss_German = 11,
  Swiss,
  Swedish = 12,
  Norwegian_Danish = 13,
  French = 14,
  Spanish = 15,
  Portugese = 16,
  Hebrew = 17,
  British_Latin_1,
  Cyrillic,
  DEC_Alt_Chars,
  DEC_Alt_Graphics,
  DEC_Spec_Graphic,
  DEC_Supp,
  DEC_Supp_Graphic,
  DEC_Tech,
  Greek,
  Greek_DEC,
  Greek_Supp,
  Hebrew_DEC,
  Hebrew_Supp,
  Latin_5_Supp,
  Latin_Cyrillic,
  Russian,
  Turkish,
  Turkish_DEC,
  SCS_NRCS,
  Unknown
} National;

typedef struct {
  National code;                /* internal name (chosen to sort 'name' member) */
  int allow96;                  /* flag for 96-character sets (e.g., GR mapping) */
  int order;                    /* check-column so we can mechanically-sort this table */
  int first;                    /* first model: 0=base, 2=vt220, 3=vt320, etc. */
  int last;                     /* last model: 0=base, 2=vt220, 3=vt320, etc. */
  const char *final;            /* end of SCS string */
  const char *name;             /* the string we'll show the user */
  const char *not11;            /* cells which are not 1-1 with ISO-8859-1 */
} CHARSETS;
/* *INDENT-OFF* */

/* compare mappings using only 7-bits */
#define Not11(a,b) (((a) & 0x7f) == ((b) & 0x7f))

/*
 * The "map_XXX" strings list the characters that should be replaced for the
 * given NRCS.  Use that to highlight them for clarity.
 */
static const char map_pound[] = "#";
static const char map_all94[] = "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
static const char map_all96[] = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\177";
static const char map_DEC_Supp[] = "$&,-./48>GHIJKLMNOPW^pw}~";
static const char map_Spec_Graphic[] = "`abcdefghijklmnopqrstuvwxyz{|}~";
static const char map_Supp_Graphic[] = "$&,-./48>PW^p}~\177";
static const char map_Dutch[] = "#@[\\]{|}~";
static const char map_Finnish[] = "`[\\]^{|}~";
static const char map_French[] = "#@[\\]{|}~";
static const char map_French_Canadian[] = "@`[\\]^{|}~";
static const char map_German[] = "@[\\]{|}~";
static const char map_Greek[] = "abcdefghijklmnopqrstuvwxyz";
static const char map_Hebrew[] = "`abcdefghijklmnopqrstuvwxyz";
static const char map_Italian[] = "#@`[\\]{|}~";
static const char map_Norwegian[] = "@`[\\]^{|}~";
static const char map_Portuguese[] = "[\\]{|}";
static const char map_Spanish[] = "@[\\]{|}";
static const char map_Swedish[] = "@`[\\]^{|}~";
static const char map_Swiss[] = "#@`[\\]^_{|}~";
static const char map_Turkish[] = "&@[\\]^{|}~";

static const CHARSETS KnownCharsets[] = {
  { ASCII,             0, 0, 0, 9, "B",    "US ASCII", 0 },
  { British,           0, 0, 0, 9, "A",    "British", map_pound },
  { British_Latin_1,   1, 0, 3, 9, "A",    "ISO Latin-1", 0 },
  { Cyrillic,          0, 0, 5, 9, "&4",   "Cyrillic (DEC)", 0 },
  { DEC_Spec_Graphic,  0, 0, 0, 9, "0",    "DEC Special graphics and line drawing", map_Spec_Graphic },
  { DEC_Alt_Chars,     0, 0, 0, 0, "1",    "DEC Alternate character ROM standard characters", 0 },
  { DEC_Alt_Graphics,  0, 0, 0, 0, "2",    "DEC Alternate character ROM special graphics", 0 },
  { DEC_Supp,          0, 0, 2, 9, "<",    "DEC Supplemental", map_DEC_Supp },
  { DEC_Supp_Graphic,  0, 0, 3, 9, "%5",   "DEC Supplemental Graphic", map_Supp_Graphic },
  { DEC_Tech,          0, 0, 3, 9, ">",    "DEC Technical", map_all94 },
  { Dutch,             0, 0, 2, 9, "4",    "Dutch", map_Dutch },
  { Finnish,           0, 0, 2, 9, "5",    "Finnish", map_Finnish },
  { Finnish,           0, 1, 2, 9, "C",    "Finnish", map_French },
  { French,            0, 0, 2, 9, "R",    "French", map_French },
  { French,            0, 1, 2, 9, "f",    "French", map_French }, /* Kermit (vt340 model?) */
  { French_Canadian,   0, 0, 2, 9, "Q",    "French Canadian", map_French_Canadian },
  { French_Canadian,   0, 1, 3, 9, "9",    "French Canadian", map_French_Canadian },
  { German,            0, 0, 2, 9, "K",    "German", map_German },
  { Greek,             0, 0, 5, 9, "\">",  "Greek", map_Greek },
  { Greek_DEC,         0, 0, 5, 9, "\"?",  "Greek (DEC)", map_all94 },
  { Greek_Supp,        1, 0, 5, 9, "F",    "ISO Greek Supplemental", map_all94 },
  { Hebrew,            0, 0, 5, 9, "%=",   "Hebrew", map_Hebrew },
  { Hebrew_DEC,        0, 0, 5, 9, "\"4",  "Hebrew (DEC)", map_all94 },
  { Hebrew_Supp,       1, 0, 5, 9, "H",    "ISO Hebrew Supplemental", map_all94 },
  { Italian,           0, 0, 2, 9, "Y",    "Italian", map_Italian },
  { Latin_5_Supp,      1, 0, 5, 9, "M",    "ISO Latin-5 Supplemental", map_all96 },
  { Latin_Cyrillic,    1, 0, 5, 9, "L",    "ISO Latin-Cyrillic", map_all96 },
  { Norwegian_Danish,  0, 0, 3, 9, "`",    "Norwegian/Danish", map_Norwegian },
  { Norwegian_Danish,  0, 1, 2, 9, "E",    "Norwegian/Danish", map_Norwegian },
  { Norwegian_Danish,  0, 2, 2, 9, "6",    "Norwegian/Danish", map_Norwegian },
  { Portugese,         0, 0, 3, 9, "%6",   "Portugese", map_Portuguese },
  { Russian,           0, 0, 5, 9, "&5",   "Russian", 0 },
  { SCS_NRCS,          0, 0, 5, 9, "%3",   "SCS", 0 },
  { Spanish,           0, 0, 2, 9, "Z",    "Spanish", map_Spanish },
  { Swedish,           0, 0, 2, 9, "7",    "Swedish", map_Swedish },
  { Swedish,           0, 1, 2, 9, "H",    "Swedish", map_Swedish },
  { Swiss,             0, 0, 2, 9, "=",    "Swiss", map_Swiss },
  { Turkish,           0, 0, 5, 9, "%2",   "Turkish", map_Turkish },
  { Turkish_DEC,       0, 0, 5, 9, "%0",   "Turkish (DEC)", map_all94 },
  { Unknown,           0, 0,-1,-1, "?",    "Unknown", 0 }
};
/* *INDENT-ON* */

static int hilite_not11;
static int national;
static int cleanup;

static char sgr_hilite[10];
static char sgr_reset[10];

static int current_Gx[4];

static int
lookupCode(National code)
{
  int n;
  for (n = 0; n < TABLESIZE(KnownCharsets); n++) {
    if (KnownCharsets[n].code == code)
      return n;
  }
  return lookupCode(ASCII);
}

static const CHARSETS *
lookupCharset(int g, int n)
{
  const CHARSETS *result = 0;
  if (n >= 0 && n < TABLESIZE(KnownCharsets)) {
    if (!strcmp(KnownCharsets[n].final, "A")) {
      if (national || (g == 0)) {
        n = lookupCode(British);
      } else {
        n = lookupCode(British_Latin_1);
      }
    }
    result = &KnownCharsets[n];
  }
  return result;
}

static const char *
charset_name(int g, int n)
{
  return lookupCharset(g, n)->name;
}

static int
append_sgr(char *buffer, int used, const char *sgr_string)
{
  strcpy(buffer + used, sgr_string);
  used += (int) strlen(sgr_string);
  return used;
}

static void
send32(int row, int upper, const char *not11)
{
  int col;
  int used = 0;
  int hilited = 0;
  char buffer[33 * 8];

  if (LOG_ENABLED) {
    fprintf(log_fp, "Note: send32 row %d, upper %d, not11:%s\n",
            row, upper, not11 ? not11 : "");
  }
  for (col = 0; col <= 31; col++) {
    char ch = (char) (row * 32 + upper + col);
    if (not11 != 0 && hilite_not11) {
      const char *p;
      int found = 0;
      for (p = not11; *p; ++p) {
        if (Not11(*p, ch)) {
          found = 1;
          break;
        }
      }
      if (found) {
        if (!hilited) {
          used = append_sgr(buffer, used, sgr_hilite);
          hilited = 1;
        }
      } else {
        if (hilited) {
          used = append_sgr(buffer, used, sgr_reset);
          hilited = 0;
        }
      }
    }
    buffer[used++] = ch;
  }
  if (hilited) {
    used = append_sgr(buffer, used, sgr_reset);
  }
  buffer[used] = 0;
  tprintf("%s", buffer);
}

static char *
scs_params(char *dst, int g)
{
  const CHARSETS *tbl = lookupCharset(g, current_Gx[g]);

  sprintf(dst, "%c%s",
          ((tbl->allow96 && get_level() > 2)
           ? "?-./"[g]
           : "()*+"[g]),
          tbl->final);
  return dst;
}

static void
do_scs(int g)
{
  char buffer[80];

  esc(scs_params(buffer, g));
}

/* reset given Gg back to sane setting */
static int
sane_cs(int g)
{
  return lookupCode(((g == 0) || (get_level() <= 1))
                    ? ASCII
                    : (get_level() < 3
                       ? British
                       : British_Latin_1));   /* ...to get 8-bit codes 128-255 */
}

/* reset given Gg back to sane setting */
static int
reset_scs(int g)
{
  int n = sane_cs(g);
  do_scs(n);
  return n;
}

/* reset all of the Gn to sane settings */
static int
reset_charset(MENU_ARGS)
{
  int n;

  (void)the_title;
  decnrcm(national = FALSE);
  for (n = 0; n < 4; n++) {
    int m = sane_cs(cleanup ? 0 : n);
    if (m != current_Gx[n] || (m == 0)) {
      current_Gx[n] = m;
      do_scs(n);
    }
  }
  return MENU_NOHOLD;
}

static int the_code;
static int the_list[TABLESIZE(KnownCharsets) + 2];

static int
lookup_Gx(MENU_ARGS)
{
  int n;
  the_code = -1;
  for (n = 0; n < TABLESIZE(KnownCharsets); n++) {
    if (the_list[n]
        && !strcmp(the_title, KnownCharsets[n].name)) {
      the_code = n;
      break;
    }
  }
  return MENU_NOHOLD;
}

static void
specify_any_Gx(MENU_ARGS, int g)
{
  MENU my_menu[TABLESIZE(KnownCharsets) + 2];
  int n, m;

  /*
   * Build up a menu of the character sets we will allow the user to specify.
   * There are a couple of tentative table entries (the "?" ones), which we
   * won't show in any event.  Beyond that, we limit some of the character sets
   * based on the emulation level (vt220 implements national replacement
   * character sets, for example, but not the 96-character ISO Latin-1).
   */
  for (n = m = 0; n < TABLESIZE(KnownCharsets); n++) {
    the_list[n] = 0;
    if (!strcmp(KnownCharsets[n].final, "?"))
      continue;
    if (get_level() < KnownCharsets[n].first)
      continue;
    if (get_level() > KnownCharsets[n].last)
      continue;
    if (((g == 0) || national) && KnownCharsets[n].allow96)
      continue;
    if (((g != 0) && !national) && (KnownCharsets[n].code == British))
      continue;
    if (m && !strcmp(my_menu[m - 1].description, KnownCharsets[n].name))
      continue;
    my_menu[m].description = KnownCharsets[n].name;
    my_menu[m].dispatch = lookup_Gx;
    the_list[n] = 1;
    m++;
  }
  my_menu[m].description = "";
  my_menu[m].dispatch = 0;

  do {
    vt_clear(2);
    __(title(0), println(the_title));
    __(title(2), println("Choose character-set:"));
  } while (menu(my_menu) && the_code < 0);

  current_Gx[g] = the_code;
}

static int
toggle_hilite(MENU_ARGS)
{
  (void)the_title;
  hilite_not11 = !hilite_not11;
  if (hilite_not11) {
    sprintf(sgr_hilite, "%s7m", csi_output());
    sprintf(sgr_reset, "%sm", csi_output());
  }
  return MENU_NOHOLD;
}

static int
toggle_nrc(MENU_ARGS)
{
  (void)the_title;
  national = !national;
  decnrcm(national);
  return MENU_NOHOLD;
}

static int
specify_G0(MENU_ARGS)
{
  specify_any_Gx(PASS_ARGS, 0);
  return MENU_NOHOLD;
}

static int
specify_G1(MENU_ARGS)
{
  specify_any_Gx(PASS_ARGS, 1);
  return MENU_NOHOLD;
}

static int
specify_G2(MENU_ARGS)
{
  specify_any_Gx(PASS_ARGS, 2);
  return MENU_NOHOLD;
}

static int
specify_G3(MENU_ARGS)
{
  specify_any_Gx(PASS_ARGS, 3);
  return MENU_NOHOLD;
}

static int
tst_layout(MENU_ARGS)
{
  char buffer[80];
  (void)the_title;
  return tst_keyboard_layout(scs_params(buffer, 0));
}

static int
tst_vt100_charsets(MENU_ARGS)
{
  /* Test of:
   * SCS    (Select character Set)
   */
  int i, g, count, cset;

  (void)the_title;
  __(cup(1, 10), printf("Selected as G0 (with SI)"));
  __(cup(1, 48), printf("Selected as G1 (with SO)"));
  for (count = cset = 0; count < TABLESIZE(KnownCharsets); count++) {
    const CHARSETS *tbl = KnownCharsets + count;
    if (tbl->first == 0) {
      int row = 3 + (4 * cset);

      scs(1, 'B');
      cup(row, 1);
      sgr("1");
      tprintf("Character set %s (%s)", tbl->final, tbl->name);
      sgr("0");
      for (g = 0; g <= 1; g++) {
        int set_nrc = (get_level() >= 2 && tbl->final[0] == 'A');
        if (set_nrc)
          decnrcm(TRUE);
        scs(g, (int) tbl->final[0]);
        for (i = 1; i <= 3; i++) {
          cup(row + i, 10 + 38 * g);
          send32(i, 0, tbl->not11);
        }
        if (set_nrc != national)
          decnrcm(national);
      }
      ++cset;
    }
  }
  scs_normal();
  __(cup(max_lines, 1), printf("These are the installed character sets. "));
  return MENU_HOLD;
}

static int
tst_shift_in_out(MENU_ARGS)
{
  /* Test of:
     SCS    (Select character Set)
   */
  static const char *const label[] =
  {
    "Selected as G0 (with SI)",
    "Selected as G1 (with SO)"
  };
  int i, cset;
  char buffer[80];

  (void)the_title;
  __(cup(1, 10), printf("These are the G0 and G1 character sets."));
  for (cset = 0; cset < 2; cset++) {
    const CHARSETS *tbl = lookupCharset(cset, current_Gx[cset]);
    int row = 3 + (4 * cset);

    scs(cset, 'B');
    cup(row, 1);
    sgr("1");
    tprintf("Character set %s (%s)", tbl->final, tbl->name);
    sgr("0");

    cup(row, 48);
    tprintf("%s", label[cset]);

    esc(scs_params(buffer, cset));
    for (i = 1; i <= 3; i++) {
      cup(row + i, 10);
      send32(i, 0, tbl->not11);
    }
    scs(cset, 'B');
  }
  cup(max_lines, 1);
  return MENU_HOLD;
}

#define map_g1_to_gr() esc("~")   /* LS1R */

static int
tst_vt220_locking(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static const struct {
    int upper;
    int mapped;
    const char *code;
    const char *msg;
  } table[] = {
    { 1, 1, "~", "G1 into GR (LS1R)" },
    { 0, 2, "n", "G2 into GL (LS2)"  }, /* "{" vi */
    { 1, 2, "}", "G2 into GR (LS2R)" },
    { 0, 3, "o", "G3 into GL (LS3)"  },
    { 1, 3, "|", "G3 into GR (LS3R)" },
  };
  /* *INDENT-ON* */

  int i, cset;

  (void)the_title;
  __(cup(1, 10), tprintf("Locking shifts, with NRC %s:",
                         STR_ENABLED(national)));
  for (cset = 0; cset < TABLESIZE(table); cset++) {
    int row = 3 + (4 * cset);
    int map = table[cset].mapped;
    const CHARSETS *tbl = lookupCharset(map, current_Gx[map]);
    int map_gl = (strstr(table[cset].msg, "into GL") != 0);

    scs_normal();
    cup(row, 1);
    sgr("1");
    tprintf("Character set %s (%s) in G%d", tbl->final, tbl->name, map);
    sgr("0");

    cup(row, 48);
    tprintf("Maps %s", table[cset].msg);

    for (i = 1; i <= 3; i++) {
      if (table[cset].upper) {
        scs_normal();
        map_g1_to_gr();
      } else {
        do_scs(map);
        esc(table[cset].code);
      }
      cup(row + i, 5);
      send32(i, 0, map_gl ? tbl->not11 : 0);

      if (table[cset].upper) {
        do_scs(map);
        esc(table[cset].code);
      } else {
        scs_normal();
        /*
         * That set G1 to ASCII, just like G0.  While that works for VT100 (a
         * 7-bit device), the later 8-bit models use Latin-1 for the default
         * value of G1.
         */
        switch (get_level()) {
        case 0:
        case 1:
          break;
        case 2:
          esc(")A");  /* select the 94-character NRCS, closest to MCS */
          break;
        default:
          esc("-A");  /* select the 96-character set */
          break;
        }
        map_g1_to_gr();
      }
      cup(row + i, 40);
      send32(i, 128, map_gl ? 0 : tbl->not11);
    }
    reset_scs(cset);
  }
  scs_normal();
  cup(max_lines, 1);
  return MENU_HOLD;
}

static int
tst_vt220_single(MENU_ARGS)
{
  int pass, x, y;

  (void)the_title;
  for (pass = 0; pass < 2; pass++) {
    int g = pass + 2;
    const CHARSETS *tbl = lookupCharset(g, current_Gx[g]);

    vt_clear(2);
    cup(1, 1);
    tprintf("Testing single-shift G%d into GL (SS%d) with NRC %s\n",
            g, g, STR_ENABLED(national));
    tprintf("G%d is %s", g, tbl->name);

    do_scs(g);
    for (y = 0; y < 16; y++) {
      for (x = 0; x < 6; x++) {
        int ch = y + (x * 16) + 32;
        int hilited = 0;

        cup(y + 5, (x * 12) + 5);
        tprintf("%3d: (", ch);
        esc(pass ? "O" : "N");  /* SS3 or SS2 */
        if (tbl->not11 && hilite_not11) {
          const char *p;
          for (p = tbl->not11; *p; ++p) {
            if (Not11(*p, ch)) {
              tprintf("%s", sgr_hilite);
              hilited = 1;
              break;
            }
          }
        }
        tprintf("%c", ch);
        if (ch == 127 && !tbl->allow96)
          tprintf(" ");   /* DEL should have been eaten - skip past */
        if (hilited) {
          tprintf("%s", sgr_reset);
        }
        tprintf(")");
      }
    }

    cup(max_lines, 1);
    holdit();
  }

  return MENU_NOHOLD;
}

/******************************************************************************/

/*
 * For parsing DECCIR response.  The end of the response consists of so-called
 * intermediate and final bytes as used by the SCS controls.  Most of the
 * strings fit into that description, but note that '<', '=' and '>' do not,
 * since they are used to denote private parameters rather than final bytes.
 * (But ECMA-48 hedges this by stating that the format in those cases is not
 * specified).
 */
const char *
parse_Sdesig(const char *source, int *offset)
{
  int j;
  const char *first = source + (*offset);
  const char *result = 0;
  size_t limit = strlen(first);

  for (j = 0; j < TABLESIZE(KnownCharsets); ++j) {
    if (KnownCharsets[j].code != Unknown) {
      size_t check = strlen(KnownCharsets[j].final);
      if (check <= limit
          && !strncmp(KnownCharsets[j].final, first, check)) {
        result = KnownCharsets[j].name;
        *offset += (int) check;
        break;
      }
    }
  }
  if (result == 0) {
    static char temp[80];
    sprintf(temp, "? %#x\n", *source);
    *offset += 1;
    result = temp;
  }
  return result;
}

/*
 * Reset G0 to ASCII
 * Reset G1 to ASCII
 * Shift-in.
 */
void
scs_normal(void)
{
  scs(0, 'B');
}

/*
 * Set G0 to Line Graphics
 * Reset G1 to ASCII
 * Shift-in.
 */
void
scs_graphics(void)
{
  scs(0, '0');
}

int
tst_characters(MENU_ARGS)
{
  static char whatis_Gx[4][80];
  static char hilite_mesg[80];
  static char nrc_mesg[80];
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Reset (ASCII for G0, G1, no NRC mode)",             reset_charset },
      { hilite_mesg,                                         toggle_hilite },
      { nrc_mesg,                                            toggle_nrc },
      { whatis_Gx[0],                                        specify_G0 },
      { whatis_Gx[1],                                        specify_G1 },
      { whatis_Gx[2],                                        specify_G2 },
      { whatis_Gx[3],                                        specify_G3 },
      { "Test VT100 Character Sets",                         tst_vt100_charsets },
      { "Test Shift In/Shift Out (SI/SO)",                   tst_shift_in_out },
      { "Test VT220 Locking Shifts",                         tst_vt220_locking },
      { "Test VT220 Single Shifts",                          tst_vt220_single },
      { "Test Soft Character Sets",                          not_impl },
      { "Test Keyboard Layout with G0 Selection",            tst_layout },
      { "",                                                  0 }
  };
  /* *INDENT-ON* */

  cleanup = 0;
  hilite_not11 = 1;
  toggle_hilite(PASS_ARGS);
  reset_charset(PASS_ARGS);   /* make the menu consistent */

  if (get_level() > 1 || input_8bits || output_8bits) {
    int n;

    do {
      vt_clear(2);
      __(title(0), printf("Character-Set Tests"));
      __(title(2), println("Choose test type:"));
      sprintf(hilite_mesg, "%s highlighting of non-ISO-8859-1 mapping",
              STR_ENABLE(hilite_not11));
      sprintf(nrc_mesg, "%s National Replacement Character (NRC) mode",
              STR_ENABLE(national));
      for (n = 0; n < 4; n++) {
        sprintf(whatis_Gx[n], "Specify G%d (now %s)",
                n, charset_name(n, current_Gx[n]));
      }
    } while (menu(my_menu));
    cleanup = 1;
    /* tidy in case a "vt100" emulator does not ignore SCS */
    vt_clear(1);
    return reset_charset(PASS_ARGS);
  } else {
    return tst_vt100_charsets(PASS_ARGS);
  }
}
