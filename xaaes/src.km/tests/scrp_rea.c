#include "aestests.h"

int main(void)
{
	char scrap_path[128];

	appl_init();

	scrp_read(scrap_path);
	(void) Cconws(scrap_path);
	appl_exit();

	return 0;
}
