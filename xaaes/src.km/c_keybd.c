#include "c_keybd.h"
#include "k_keybd.h"
#include "xa_types.h"

void
cXA_fmdkey(enum locks lock, struct c_event *ce)
{
	struct xa_client *client = ce->client;
	struct rawkey *key = (struct rawkey *)ce->ptr1;

	DIAG((D_keybd, client, "Deliver fmd.keybress to %s", client->name));

	client->fmd.keypress(lock, NULL, &client->wt, key->aes, key->norm, *key);
	kfree(key);
}
void
cXA_keypress(enum locks lock, struct c_event *ce)
{
	struct xa_client *client = ce->client;
	struct rawkey *key = (struct rawkey *)ce->ptr1;
	struct xa_window *wind = (struct xa_window *)ce->ptr2;

	DIAG((D_keybd, client, "cXA_keypress for %s", client->name));

	wind->keypress(lock, wind, NULL, key->aes, key->norm, *key);
	kfree(key);
}
void
cXA_keybd_event(enum locks lock, struct c_event *ce)
{
	struct xa_client *client = ce->client;
	struct rawkey *key = (struct rawkey *)ce->ptr1;

	keybd_event(lock, client, key);
	kfree(key);
}