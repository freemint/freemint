#include "aestests.h"

int main(void)
{
	char tail[] = "\xff"
		"sis is e test "
		"dies ist ein Text "
		"ille est une teste "
		"este es un testo "
		"dobr dinske iste teste "
		"ching hong ping test "
		"pruschni gruschni tesni "
		"blubbi labi suelzi";
	int doex,
	 isgr;

	appl_init();
	doex = 1;
	isgr = TRUE;
	/* shel_write for MTOS */
	shel_write(doex, isgr, TRUE, "c:\\bin\\cmdline.prg", tail);
	appl_exit();
	return 0;
}
