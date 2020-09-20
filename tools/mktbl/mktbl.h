#define TAB_UNSHIFT   0
#define TAB_SHIFT     1
#define TAB_CAPS      2
#define TAB_ALTGR     3
#define TAB_SHALTGR   4
#define TAB_CAPSALTGR 5
#define TAB_ALT       6
#define TAB_ALTSHIFT  7
#define TAB_ALTCAPS   8
#define N_KEYTBL      9
#define TAB_DEADKEYS  N_KEYTBL

#define MAX_SCANCODE 128

#define MAXAKP                126     /* maximum _AKP code supported */

/*
 * order of tables in MagiC
 */
struct keytab {
	unsigned char *unshift;		/* every TOS */
	unsigned char *shift;		/* every TOS */
	unsigned char *caps;		/* every TOS */
	unsigned char *altgr;		/* Milan TOS 4.06/MiNT/MagiC */
	unsigned char *shaltgr;		/* MagiC */
	unsigned char *capsaltgr;	/* MagiC */
	unsigned char *alt;			/* TOS 4.0x and above/MiNT/MagiC/EmuTOS */
	unsigned char *altshift;	/* TOS 4.0x and above/MiNT/MagiC/EmuTOS */
	unsigned char *altcaps;		/* TOS 4.0x and above/MiNT/MagiC/EmuTOS */
	unsigned char *deadkeys;	/* FreeMiNT 1.17/MagiC/EmuTOS */
};

/*
 * order of tables in TOS/MiNT
 */
struct mint_keytab {
	unsigned char *unshift;		/* every TOS */
	unsigned char *shift;		/* every TOS */
	unsigned char *caps;		/* every TOS */
	unsigned char *alt;			/* TOS 4.0x and above/MiNT/MagiC */
	unsigned char *altshift;	/* TOS 4.0x and above/MiNT/MagiC */
	unsigned char *altcaps;		/* TOS 4.0x and above/MiNT/MagiC */
	unsigned char *altgr;		/* Milan TOS 4.06/MiNT/MagiC */
	unsigned char *deadkeys;	/* FreeMiNT 1.17/MagiC */
};

#define MAX_DEADKEYS 2048
extern unsigned char keytab[N_KEYTBL][MAX_SCANCODE];
extern unsigned char deadkeys[MAX_DEADKEYS];
extern int tabsize[N_KEYTBL + 1];

/*
 * input formats, both binary and source
 */
#define FORMAT_NONE     0
#define FORMAT_MAGIC    1
#define FORMAT_MINT     2
/*
 * output formats, only used by conversion tool
 */
#define FORMAT_C_SOURCE     3
#define FORMAT_MINT_SOURCE  4
#define FORMAT_MAGIC_SOURCE 5

/*
 * Format of the original input file.
 * For simpler processing, alt-tables are parsed into
 * full 128-bytes tables
 */
extern int keytab_format;
/*
 * Format of the deadkeys. This is left alone.
 */
extern int deadkeys_format;
/*
 * Country code of the loaded table, or -1
 */
extern int keytab_ctry_code;
/*
 * index into the ISO code for the table, or 0
 */
extern int keytab_codeset;

extern unsigned short const keytab_codesets[];

int mktbl_parse(FILE *fd, const char *filename);
void conv_magic_deadkeys(const unsigned char *src);
void copy_mint_deadkeys(const unsigned char *src);
int conv_mint_deadkeys(unsigned char *tmp);
const unsigned char *conv_table(int tab, const unsigned char *src);
void mktbl_write_c_src(FILE *out);
int mktbl_write_mint(FILE *out);
int mktbl_write_magic(FILE *out);
int mktbl_write_mint_source(FILE *out);
int mktbl_write_magic_source(FILE *out);
int is_deadkey(unsigned char c);
int lookup_codeset(unsigned short magic);
