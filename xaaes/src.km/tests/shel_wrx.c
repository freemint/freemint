#include "aestests.h"

int main(void)
{
	long xargs[5];

	char env[] = "mist=dreck\0dumm=doof\0";

	char tail[] = "\xff"
		"sis is e test "
		"dies ist ein Text "
		"ille est une teste "
		"este es un testo "
		"dobr dinske iste teste "
		"ching hong ping test "
		"pruschni gruschni tesni "
		"blubbi labi suelzi";
/*
	char tail[] = "\xff"	"sis is e test "
						"dies ist ein Text "
						"ille est une teste "
						"blubbi labi suelzi";
*/

	int doex, isgr;

	xargs[0] = (long) "c:\\bin\\cmdline.tos";
	xargs[1] = 0L;						/* limit */
	xargs[2] = 0L;						/* nice */
	xargs[3] = (long) "";				/* path */
	xargs[4] = (long) env;

	appl_init();
	doex = TRUE | 2048;
	isgr = FALSE;
	shel_write(doex, isgr, SHW_PARALLEL, (char *) xargs, tail);
	return 0;
}
