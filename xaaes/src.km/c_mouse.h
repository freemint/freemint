#ifndef	_c_mouse_h
#define _c_mouse_h

#include "xa_types.h"

void cXA_button_event(enum locks lock, struct c_event *ce);
void cXA_form_do(enum locks lock, struct c_event *ce);
void cXA_menu_move(enum locks lock, struct c_event *ce);
void cXA_active_widget(enum locks lock, struct c_event *ce);
void cXA_widget_click(enum locks lock, struct c_event *ce);

#endif /* _c_mouse_h */