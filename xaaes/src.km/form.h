#ifndef	_form_h
#define _form_h

#include "xa_types.h"
#include "mt_gem.h"

void	Set_form_do	(struct xa_client *client,
			 OBJECT *obtree,
			 short edobj);

bool	Setup_form_do	(struct xa_client *client,
			 OBJECT *obtree,
			 short edobj,
			 /* Output */
			 struct xa_window **ret_wind,
			 short *ret_edobj);

void	form_center	(OBJECT *obtree, short barsizes);

bool	form_button	(XA_TREE *wt,
			 short obj,
			 const struct moose_data *md,
			 bool redraw,
			 struct xa_rect_list *rl,
			 /* Outputs */
			 short *newstate,
			 short *nxtob,
			 short *clickmsk);

short	form_cursor	(XA_TREE *wt,
			 ushort keycode,
			 short obj,
			 bool redraw,
			 struct xa_rect_list *rl);

bool	form_keyboard	(XA_TREE *wt,
			 short obj,
			 const struct rawkey *key,
			 bool redraw,
			 struct xa_rect_list *rl,
			 /* outputs */
			 short *nxtobj,
			 short *newstate,
			 short *nxtkey);

void	Exit_form_do	(struct xa_client *client,
			 struct xa_window *wind,
			 XA_TREE *wt,
			 struct fmd_result *fr);

void set_button_timer(enum locks lock, struct xa_window *wind);

WidgetBehaviour	Click_windowed_form_do;
FormMouseInput	Click_form_do;
FormKeyInput	Key_form_do;
//SendMessage	handle_form_window;
DoWinMesag	do_formwind_msg;

#endif /* _form_h */
