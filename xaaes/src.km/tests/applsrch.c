#include "aestests.h"

int main(void)
{
	char apname[16];
	_WORD i;
	_WORD aptyp;
	_WORD apid;

	appl_init();

	/* sfirst */
	i = appl_search(0, apname, &aptyp, &apid);
	while (i)
	{
		printf("Name = \"%s\" typ = %2d apid = %d\n", apname, aptyp, apid);
		i = appl_search(1, apname, &aptyp, &apid);
	}

	appl_exit();
	return 0;
}
