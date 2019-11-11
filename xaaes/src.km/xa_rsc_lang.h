#define MAX_TITLES 32

struct rsc_scan
{
	short n_titles, title, lastbox, mwidth, keep_w, change_lo[MAX_TITLES], shift[MAX_TITLES];
	char *titles[MAX_TITLES];
};

short rsc_trans_rw(struct xa_client *client, XA_FILE *rfp, char **txt, short *chgl);
void rsc_lang_translate_object(struct xa_client *client, XA_FILE *rfp, OBJECT *obj, struct rsc_scan *scan);

XA_FILE *rsc_lang_file_open(const char *name);
XA_FILE *rsc_lang_file_create(const char *name);
void rsc_lang_file_write(XA_FILE *fp, const char *buf, long l);
