#include "c_message.h"
#include "xa_types.h"
#include "messages.h"

void
cXA_deliver_msg(enum locks lock, struct c_event *ce)
{
	DIAG((D_m, ce->client, "Deliver msg to %s", ce->client->name));
	deliver_message(lock, ce->client, ce->ptr1);
	kfree(ce->ptr1);
}
