#define DEV_TYPE_ROOT_HUB 0
#define DEV_TYPE_HUB 1
#define DEV_TYPE_GENERAL 2
#define DEV_TYPE_ERROR 3

/*    Prototypes : */

void events (short menuID);
void update_text(void);
void init (void);
void open_text(void);
void close_text(void);
void redraw(short w_handle, short x, short y, short w, short h);
void sized(short w_handle, short x, short y, short w, short h);
void fulled(void);
void arrow(short buff4);
void vslider(short w_handle, short x, short y, short w, short h);
void hslider(short w_handle, short x, short y, short w, short h);
void sliders(void);
void set_clip(short clip_flag, GRECT *area);
void n_string(char *s, int n);
