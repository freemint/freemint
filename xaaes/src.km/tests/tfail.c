/*
 *
 * This program always answers AP_TFAIL
 * when receiving an AP_TERM message
 *
 */

#include "aestests.h"

char text[128];

int main(void)
{
	_WORD buf[8];

	appl_init();

	/* tell AES: i understand AP_TERM */
	shel_write(SHW_INFRECGN, 1, 0, NULL, NULL);

	for (;;)
	{
		evnt_mesag(buf);

		if (buf[0] == AP_TERM)
		{
			sprintf(text, "[1][AP_TERM.|Reason: %d][Ok]", buf[5]);
			buf[0] = AP_TFAIL;
			buf[1] = 4711;
			shel_write(SHW_AESSEND, 0, 0, (void *) buf, NULL);
			form_alert(1, text);
		} else
		{
			sprintf(text, "[1][Unknowm message received|Code %d[Cancel]", buf[0]);
			form_alert(1, text);
			appl_exit();
			return 2;
		}
	}
}
