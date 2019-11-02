#include "aestests.h"

int main(void)
{
	_WORD message[8];

	appl_init();
	for (;;)
	{
		evnt_mesag(message);
		if (message[0] == AP_TERM)
		{
			break;
		}
	}
	appl_exit();
	return 0;
}
