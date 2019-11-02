#include "aestests.h"

int main(void)
{
	_WORD r;
	_WORD wc, hc, wb, hb;

	r = graf_handle(&wc, &hc, &wb, &hb);
	printf("%d %d %d %d %d\n", r, wc, hc, wb, hb);
	printf("press key");
	fflush(stdout);
	Cnecin();
	return 0;
}
