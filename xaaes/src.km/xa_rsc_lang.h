#define MAX_TITLES 32

struct rsc_scan
{
	short n_titles, title, lastbox, mwidth, keep_w, change_lo[MAX_TITLES], shift[MAX_TITLES];
	char *titles[MAX_TITLES];
};

XA_FILE *rsc_lang_file( int md, XA_FILE *fp, char *buf, long l );
short rsc_trans_rw( struct xa_client *client, XA_FILE *rfp, char **txt, long l );
void rsc_lang_translate_xaaes_strings(struct xa_client *client, XA_FILE *rfp);
void rsc_lang_translate_object(struct xa_client *client, XA_FILE *rfp, OBJECT *obj, struct rsc_scan *scan);
