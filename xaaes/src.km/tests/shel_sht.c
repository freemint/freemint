#include "aestests.h"

int main(void)
{
	appl_init();
	/* we understand AP_TERM */
	shel_write(SHW_INFRECGN, 1, 0, (char *) 0, (char *) 0);
	for (;;)
		evnt_timer(0);
}
