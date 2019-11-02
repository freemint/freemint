#include "aestests.h"

int main(void)
{
	char path[128] = "blub.tst";
	int ret;

	appl_init();
	ret = shel_find(path);
	(void) Cconws(path);
	if (!ret)
		(void) Cconws("-- not found --");
	return 0;
}
