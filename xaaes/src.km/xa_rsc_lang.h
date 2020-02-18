short rsc_trans_rw(struct xa_client *client, XA_FILE *rfp, char **txt, bool *strip);
void rsc_lang_translate_object(struct xa_client *client, XA_FILE *rfp, OBJECT *obj, bool *strip);
void rsc_lang_fix_menu(struct xa_client *client, OBJECT *menu, XA_FILE *rfp);

XA_FILE *rsc_lang_file_open(const char *name);
XA_FILE *rsc_lang_file_create(const char *name);
void rsc_lang_file_write(XA_FILE *fp, const char *buf, long l);
