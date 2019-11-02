/*
 * Test for message buffer overflow.
 * Sends messages to an arbitrary program whose
 * application id is passed
 */

#include "aestests.h"

int main(int argc, char *argv[])
{
	_WORD id;
	_WORD buf[8];

	appl_init();
	if (argc != 2)
	{
		form_alert(1, "[3][Syntax: MSG_OVL ap_id][Cancel]");
		return 1;
	}

	id = atoi(argv[1]);
	for (;;)
	{
		if (appl_write(id, 16, buf) == 0)
			break;
	}
	appl_exit();
	return 0;
}
