/* tab 8 */

# ifndef SLB_GEMMA_H
# define SLB_GEMMA_H

# include <mt_gem.h>
# include <mint/slb.h>
# include <slb/gemma-struct.h>
# include <slb/kernel.h>

#undef SLB_NWORDS
#define SLB_NWORDS(_nargs) ((((long)(_nargs) * 2 + 1l) << 16) | (long)(_nargs))
#undef SLB_NARGS
#define SLB_NARGS(_nargs) SLB_NWORDS(_nargs)

/* SLB function declarations for libgemma.a */

#ifndef _CDECL
#  define _CDECL
#endif

extern long _startup(void);
extern short call_aes(GEM_ARRAY *gemstr, short fn);
extern GEM_ARRAY *gem_control(void);
extern long appl_open(const char *file, short thread, const char *pname);
extern long appl_close(void);
extern SLB *get_gemma_p(void) __attribute__((__const__));

/* SLB macros */

static inline
GEM_ARRAY *gem_contrl(void)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long) = (long _CDECL (*)(SLB_HANDLE, long, long))gemma_p->exec;

	return (GEM_ARRAY *)(*exec)(gemma_p->handle, GEM_CTRL, SLB_NARGS(0));
}

static inline
long appl_top(void)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long) = (long _CDECL (*)(SLB_HANDLE, long, long))gemma_p->exec;

	return (*exec)(gemma_p->handle, AP_TOP, SLB_NARGS(0));
}

static inline
long rsrc_xload(const char *file)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, const char *) = (long _CDECL (*)(SLB_HANDLE, long, long, const char *))gemma_p->exec;

	return (*exec)(gemma_p->handle, RSRC_XLOAD, SLB_NARGS(1), file);
}

static inline
long rsrc_xalloc(void)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long) = (long _CDECL (*)(SLB_HANDLE, long, long))gemma_p->exec;

	return (*exec)(gemma_p->handle, RSRC_XALLOC, SLB_NARGS(0));
}

static inline
long rsrc_xfree(void)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long) = (long _CDECL (*)(SLB_HANDLE, long, long))gemma_p->exec;

	return (*exec)(gemma_p->handle, RSRC_XFREE, SLB_NARGS(0));
}

static inline
long rsrc_xgaddr(short type, short obj)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, long, long) = (long _CDECL (*)(SLB_HANDLE, long, long, long, long))gemma_p->exec;

	return (*exec)(gemma_p->handle, RSRC_XGADDR, SLB_NARGS(2), type, obj);
}

static inline
const char *env_get(const char *var)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, const char *) = (long _CDECL (*)(SLB_HANDLE, long, long, const char *))gemma_p->exec;

	return (const char *)(*exec)(gemma_p->handle, ENV_GET, SLB_NARGS(1), var);
}

static inline
char *env_eval(const char *var)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, const char *) = (long _CDECL (*)(SLB_HANDLE, long, long, const char *))gemma_p->exec;

	return (char *)(*exec)(gemma_p->handle, ENV_EVAL, SLB_NARGS(1), var);
}

static inline
long env_getargc(void)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long) = (long _CDECL (*)(SLB_HANDLE, long, long))gemma_p->exec;

	return (*exec)(gemma_p->handle, ENV_GETARGC, SLB_NARGS(0));
}

static inline
char *env_getargv(long argc)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, long) = (long _CDECL (*)(SLB_HANDLE, long, long, long))gemma_p->exec;

	return (char *)(*exec)(gemma_p->handle, ENV_GETARGV, SLB_NARGS(1), argc);
}

static inline
long windial_size(void)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long) = (long _CDECL (*)(SLB_HANDLE, long, long))gemma_p->exec;

	return (*exec)(gemma_p->handle, WD_FIELD, SLB_NARGS(0));
}

static inline
WINDIAL *windial_create(WINDIAL *wd, short box, short icon, short ftext, const char *title)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, WINDIAL *, long, long, long, const char *) = (long _CDECL (*)(SLB_HANDLE, long, long, WINDIAL *, long, long, long, const char *))gemma_p->exec;

	return (WINDIAL *)(*exec)(gemma_p->handle, WD_CREATE, SLB_NARGS(5), wd, box, icon, ftext, title);
}

static inline
WINDIAL *windial_xcreate(WINDIAL *wd, short box, short icon, short ftext, const char *title, short gadgets)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, WINDIAL *, long, long, long, const char *, long) = (long _CDECL (*)(SLB_HANDLE, long, long, WINDIAL *, long, long, long, const char *, long))gemma_p->exec;

	return (WINDIAL *)(*exec)(gemma_p->handle, WD_CREATE, SLB_NARGS(6), wd, box, icon, ftext, title, gadgets);
}

static inline
long windial_open(WINDIAL *wd)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, WINDIAL *) = (long _CDECL (*)(SLB_HANDLE, long, long, WINDIAL *))gemma_p->exec;

	return (*exec)(gemma_p->handle, WD_OPEN, SLB_NARGS(1), wd);
}

static inline
long windial_formdo(WINDIAL *wd)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, WINDIAL *) = (long _CDECL (*)(SLB_HANDLE, long, long, WINDIAL *))gemma_p->exec;

	return (*exec)(gemma_p->handle, WD_FORMDO, SLB_NARGS(1), wd);
}

static inline
long windial_close(WINDIAL *wd)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, WINDIAL *) = (long _CDECL (*)(SLB_HANDLE, long, long, WINDIAL *))gemma_p->exec;

	return (*exec)(gemma_p->handle, WD_CLOSE, SLB_NARGS(1), wd);
}

static inline
long windial_close_all(void)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long) = (long _CDECL (*)(SLB_HANDLE, long, long))gemma_p->exec;

	return (*exec)(gemma_p->handle, WD_CLALL, SLB_NARGS(0));
}

static inline
long windial_delete(WINDIAL *wd)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, WINDIAL *) = (long _CDECL (*)(SLB_HANDLE, long, long, WINDIAL *))gemma_p->exec;

	return (*exec)(gemma_p->handle, WD_DELETE, SLB_NARGS(1), wd);
}

static inline
long windial_delete_all(void)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long) = (long _CDECL (*)(SLB_HANDLE, long, long))gemma_p->exec;

	return (*exec)(gemma_p->handle, WD_DELALL, SLB_NARGS(0));
}

static inline
long windial_center(WINDIAL *wd)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, WINDIAL *) = (long _CDECL (*)(SLB_HANDLE, long, long, WINDIAL *))gemma_p->exec;

	return (*exec)(gemma_p->handle, WD_CENTER, SLB_NARGS(1), wd);
}

static inline
long windial_link(WINDIAL *from, WINDIAL *to)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, WINDIAL *, WINDIAL *) = (long _CDECL (*)(SLB_HANDLE, long, long, WINDIAL *, WINDIAL *))gemma_p->exec;

	return (*exec)(gemma_p->handle, WD_LINK, SLB_NARGS(2), from, to);
}

static inline
long windial_unlink(WINDIAL *wd)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, WINDIAL *) = (long _CDECL (*)(SLB_HANDLE, long, long, WINDIAL *))gemma_p->exec;

	return (*exec)(gemma_p->handle, WD_UNLINK, SLB_NARGS(1), wd);
}

# if 0
static inline void windial_longjmp(WINDIAL *wd, short vec)  __attribute__((__noreturn__));
# endif

static inline
void windial_longjmp(WINDIAL *wd, short vec)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, WINDIAL *, long) = (long _CDECL (*)(SLB_HANDLE, long, long, WINDIAL *, long))gemma_p->exec;

	(*exec)(gemma_p->handle, WD_RETURN, SLB_NARGS(2), wd, vec);
}

static inline __attribute__((__returns_twice__))
long windial_setjmp(WINDIAL *wd, short vec, void *adr)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, WINDIAL *, long, void *) = (long _CDECL (*)(SLB_HANDLE, long, long, WINDIAL *, long, void *))gemma_p->exec;

	return (*exec)(gemma_p->handle, WD_HANDLER, SLB_NARGS(4), wd, vec, adr);
}

static inline
long windial_alert(short button, const char *str)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, long, const char *) = (long _CDECL (*)(SLB_HANDLE, long, long, long, const char *))gemma_p->exec;

	return (*exec)(gemma_p->handle, WD_ALERT, SLB_NARGS(2), button, str);
}

static inline
void windial_error(long error, const char *msg)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, long, const char *) = (long _CDECL (*)(SLB_HANDLE, long, long, long, const char *))gemma_p->exec;

	(*exec)(gemma_p->handle, WD_ERROR, SLB_NARGS(2), error, msg);
}

static inline
long thread_fork(void *addr, const char *ptitle, long stack, long mode)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, void *, void *, const char *, long, long) = (long _CDECL (*)(SLB_HANDLE, long, long, void *, void *, const char *, long, long))gemma_p->exec;

	return (*exec)(gemma_p->handle, TFORK, SLB_NARGS(5), _startup, addr, ptitle, stack, mode);
}

static inline
long thread_overlay(void *addr, const char *ptitle, long stack, long mode)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, void *, void *, const char *, long, long) = (long _CDECL (*)(SLB_HANDLE, long, long, void *, void *, const char *, long, long))gemma_p->exec;

	return (*exec)(gemma_p->handle, TOVL, SLB_NARGS(5), _startup, addr, ptitle, stack, mode);
}

static inline
void objc_xdraw(WINDIAL *wd, short obj)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, WINDIAL *, long) = (long _CDECL (*)(SLB_HANDLE, long, long, WINDIAL *, long))gemma_p->exec;

	(*exec)(gemma_p->handle, OB_XDRAW, SLB_NARGS(2), wd, obj);
}

static inline
long av_dir_update(short drive)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, long) = (long _CDECL (*)(SLB_HANDLE, long, long, long))gemma_p->exec;

	return (*exec)(gemma_p->handle, AV_DIR_UPD, SLB_NARGS(1), drive);
}

static inline
long av_view(const char *pathname)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, const char *) = (long _CDECL (*)(SLB_HANDLE, long, long, const char *))gemma_p->exec;

	return (*exec)(gemma_p->handle, AV_XVIEW, SLB_NARGS(1), pathname);
}

static inline
long av_help(const char *fname)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, const char *) = (long _CDECL (*)(SLB_HANDLE, long, long, const char *))gemma_p->exec;

	return (*exec)(gemma_p->handle, AV_HELP, SLB_NARGS(1), fname);
}

static inline
long open_url(const char *url)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, const char *) = (long _CDECL (*)(SLB_HANDLE, long, long, const char *))gemma_p->exec;

	return (*exec)(gemma_p->handle, NET_URL, SLB_NARGS(1), url);
}

static inline
short slb_rc_intersect(const GRECT *src, GRECT *dest)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, const GRECT *, GRECT *) = (long _CDECL (*)(SLB_HANDLE, long, long, const GRECT *, GRECT *))gemma_p->exec;

	return (*exec)(gemma_p->handle, RC_INTERSECT, SLB_NARGS(2), src, dest);
}

# define rc_intersect(s,d) slb_rc_intersect(s,d)

static inline
long objc_xchange(WINDIAL *wd, short obj, short newst, short redraw)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, WINDIAL *, long, long, long) = (long _CDECL (*)(SLB_HANDLE, long, long, WINDIAL *, long, long, long))gemma_p->exec;

	return (*exec)(gemma_p->handle, OB_XCHANGE, SLB_NARGS(4), wd, obj, newst, redraw);
}

static inline
void ftext_init(short tree, short obj)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, long, long) = (long _CDECL (*)(SLB_HANDLE, long, long, long, long))gemma_p->exec;

	(*exec)(gemma_p->handle, FT_FIX, SLB_NARGS(2), tree, obj);
}

static inline
char *file_select(const char *title, const char *mask, short flag)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, const char *, const char *, long) = (long _CDECL (*)(SLB_HANDLE, long, long, const char *, const char *, long))gemma_p->exec;

	return (char *)(*exec)(gemma_p->handle, FSELINPUT, SLB_NARGS(3), title, mask, flag);
}

static inline
long menu_xpop(WINDIAL *wd, short obj, MENU *menu)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, WINDIAL *, long, MENU *) = (long _CDECL (*)(SLB_HANDLE, long, long, WINDIAL *, long, MENU *))gemma_p->exec;

	return (*exec)(gemma_p->handle, MN_XPOP, SLB_NARGS(3), wd, obj, menu);
}

static inline
long proc_exec(short mode, long flag, const char *cmd, const char *tail, const char *env)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long, long, long, const char *, const char *, const char *) = (long _CDECL (*)(SLB_HANDLE, long, long, long, long, const char *, const char *, const char *))gemma_p->exec;

	return (*exec)(gemma_p->handle, PROC_EXEC, SLB_NARGS(5), mode, flag, cmd, tail, env);
}

static inline
const char *get_version(void)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long) = (long _CDECL (*)(SLB_HANDLE, long, long))gemma_p->exec;

	return (char *)(*exec)(gemma_p->handle, GET_VERSION, SLB_NARGS(0));
}

static inline
long get_users(void)
{
	SLB *gemma_p = get_gemma_p();
	long _CDECL (*exec)(SLB_HANDLE, long, long) = (long _CDECL (*)(SLB_HANDLE, long, long))gemma_p->exec;

	return (*exec)(gemma_p->handle, GET_USERS, SLB_NARGS(0));
}

/* AES bindings for libgemma.a */

extern long appl_control(short ap_cid, short ap_cwhat, void *ap_cout);
extern long appl_exit(void);
extern long appl_find(char *name);
extern long appl_getinfo(short cwhat, short *out1, short *out2, short *out3, short *out4);
extern long appl_init(void);
extern long appl_read(short ApId, short Length, void *ApPbuff);
extern long appl_search(short mode, char *fname, short *type, short *ap_id);
extern long appl_tplay(void *Mem, short Num, short Scale);
extern long appl_trecord(void *Mem, short Count);
extern long appl_write(short ApId, short Length, void *ApPbuff);
extern long appl_yield(void);
extern long appl_bvset(short flo, short hard);

extern long evnt_button(short Clicks, short WhichButton, short WhichState, short *Mx, short *My, short *ButtonState, short *KeyState);
extern long evnt_dclick(short ToSet, short SetGet);
extern long evnt_keybd(void);
extern long evnt_mesag(short MesagBuf[]);
extern long evnt_mouse(short EnterExit, short InX, short InY, short InW, short InH, short *OutX, short *OutY, short *ButtonState, short *KeyState);
extern long evnt_multi(short Type, short Clicks, short WhichButton, short WhichState,
			short EnterExit1, short In1X, short In1Y, short In1W, short In1H,
			short EnterExit2, short In2X, short In2Y, short In2W, short In2H,
			short MesagBuf[], unsigned long Interval, short *OutX, short *OutY,
			short *ButtonState, short *KeyState, short *Key, short *ReturnCount);
extern long evnt_timer(unsigned long msec);

extern long form_alert(short DefButton, char *Str);
extern long form_button(void *Btree, short Bobject, short Bclicks, short *Bnxtobj);
extern long form_center(void *Tree, short *Cx, short *Cy, short *Cw, short *Ch);
extern long form_dial(short Flag, short Sx, short Sy, short Sw, short Sh, short Bx, short By, short Bw, short Bh);
extern long form_do(void *Tree, short StartObj);
extern long form_error(short ErrorCode);
extern long form_keybd(void *Ktree, short Kobject, short Kobnext, short Kchar, short *Knxtobject, short *Knxtchar);
extern long form_popup(void *tree, short x, short y);

extern long fsel_exinput(char *Path, char *File, short *ExitButton, char *title);
extern long fsel_input(char *Path, char *File, short *ExitButton);

extern long graf_dragbox(short Sw, short Sh, short Sx, short Sy, short Bx, short By, short Bw, short Bh, short *Fw, short *Fh);
extern long graf_growbox(short Sx, short Sy, short Sw, short Sh, short Fx, short Fy, short Fw, short Fh);
extern long graf_handle(short *Wchar, short *Hchar, short *Wbox, short *Hbox);
extern long graf_movebox(short Sw, short Sh, short Sx, short Sy, short Dx, short Dy);
extern long graf_mkstate(short *Mx, short *My, short *ButtonState, short *KeyState);
extern long graf_mouse(short Form, void *FormAddress);
extern long graf_multirubber(short x, short y, short minw, short minh, GRECT *rec, short *outw, short *outh);
extern long graf_rubberbox(short Ix, short Iy, short Iw, short Ih, short *Fw, short *Fh);
extern long graf_shrinkbox(short Fx, short Fy, short Fw, short Fh, short Sx, short Sy, short Sw, short Sh);
extern long graf_slidebox(void *Tree, short Parent, short Object, short Direction);
extern long graf_watchbox(void *Tree, short Object, short InState, short OutState);

extern long menu_attach(short me_flag, OBJECT *me_tree, short me_item, MENU *me_mdata);
extern long menu_bar(void *Tree, short ShowFlag);
extern long menu_click(short click, short setit);
extern long menu_icheck(void *Tree, short Item, short CheckFlag);
extern long menu_ienable(void *Tree, short Item, short EnableFlag);
extern long menu_istart(short me_flag, OBJECT *me_tree, short me_imenu, short me_item);
extern long menu_popup(MENU *me_menu, short me_xpos, short me_ypos, MENU *me_mdata);
extern long menu_register(short ApId, char *MenuText);
extern long menu_settings(short me_flag, MN_SET *me_values);
extern long menu_text(void *Tree, short Item, char *Text);
extern long menu_tnormal(void *Tree, short Item, short NormalFlag);
extern long menu_unregister(short apid);

extern long objc_add(void *Tree, short Parent, short Child);
extern long objc_change(void *Tree, short Object, short Res, short Cx, short Cy, short Cw, short Ch, short NewState,short Redraw);
extern long objc_delete(void *Tree, short Object);
extern long objc_draw(void *tree, short obj, short depth, short wx, short wy, short ww, short wh);
extern long objc_edit(void *tree, short obj, short ch, short idx, short mode);
extern long objc_find(void *Tree, short Start, short Depth, short Mx, short My);
extern long objc_offset(void *tree, short obj, short *x, short *y);
extern long objc_order(void *Tree, short Object, short NewPos);
extern long objc_sysvar(short mode, short which, short in1, short in2, short *out1, short *out2);
extern long objc_xfind(void *tree, short start, short depth, short x, short y);

extern long rsrc_free(void);
extern long rsrc_gaddr(short Type, short Index, void **Address);
extern long rsrc_load(char *Name);
extern long rsrc_obfix(void *Tree, short Index);
extern long rsrc_rcfix(void *rc_header);
extern long rsrc_saddr(short Type, short Index, void *Address);

extern long scrp_read(char *Scrappath);
extern long scrp_write(char *Scrappath);
extern long scrp_clear(void);

extern long shel_envrn(char **result, char *param);
extern long shel_find(char *buf);
extern long shel_get(char *Buf, short Len);
extern long shel_help(short sh_hmode, char *sh_hfile, char *sh_hkey);
extern long shel_put(char *Buf, short Len);
extern long shel_rdef(char *lpcmd, char *lpdir);
extern long shel_read(char *Command, char *Tail);
extern long shel_wdef(char *lpcmd, char *lpdir);
extern long shel_write(short Exit, short Graphic, short Aes, void *Command, char *Tail);

extern long wind_calc(short Type, short Parts, short InX, short InY, short InW, short InH, short *OutX, short *OutY, short *OutW, short *OutH);
extern long wind_close(short WindowHandle);
extern long wind_create(short Parts, short Wx, short Wy, short Ww, short Wh);
extern long wind_delete(short WindowHandle);
extern long wind_draw(short handle, short startobj);
extern long wind_find(short X, short Y);
extern long wind_get(short WindowHandle, short What, short *W1, short *W2, short *W3, short *W4);
extern void wind_new(void);
extern long wind_open(short WindowHandle, short Wx, short Wy, short Ww, short Wh);
extern long wind_set(short WindowHandle, short What, short W1, short W2, short W3, short W4);
extern long wind_sget(short handle, short field, char *buf);
extern long wind_update(short Code);

# endif

/* EOF */
