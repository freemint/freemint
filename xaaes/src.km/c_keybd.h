#ifndef _c_keybd_h
#define _c_keybd_h

#include "xa_types.h"

void cXA_fmdkey(enum locks lock, struct c_event *ce);
void cXA_keypress(enum locks lock, struct c_event *ce);
void cXA_keybd_event(enum locks lock, struct c_event *ce);

#endif /* _c_keybd_h */
