#ifndef _c_message_h
#define _c_message_h

#include "xa_types.h"

void cXA_deliver_msg(enum locks lock, struct c_event *ce);

#endif /* _c_message_h */
