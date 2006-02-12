#include <my_tos.h>
#include <bgh.h>

void main(void)
{
void *help;
	help=BGH_load("TEST.BGH");
	Cconws(BGH_gethelpstring(help,BGH_DIAL,0,56));
	Cconws("\r\n");
	Cconws(BGH_gethelpstring(help,BGH_MORE,0,44));
	Cconws("\r\n");
	BGH_free(help);
	Cnecin();
}