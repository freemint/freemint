#include "aestests.h"


int main(void)
{
	_WORD i1, i2, i3, i4;
	_WORD r;

	appl_init();
	
	i1 = i2 = i3 = i4 = 0;
	r = appl_getinfo(0, &i1, &i2, &i3, &i4);
	if (r == 0) i1 = i2 = i3 = i4 = 0;
	printf("appl_getinfo(0=AES_LARGEFONT)  : %d: %d %d %d %d\n", r, i1, i2, i3, i4);

	i1 = i2 = i3 = i4 = 0;
	r = appl_getinfo(1, &i1, &i2, &i3, &i4);
	if (r == 0) i1 = i2 = i3 = i4 = 0;
	printf("appl_getinfo(1=AES_SMALLFONT)  : %d: %d %d %d %d\n", r, i1, i2, i3, i4);

	i1 = i2 = i3 = i4 = 0;
	r = appl_getinfo(2, &i1, &i2, &i3, &i4);
	if (r == 0) i1 = i2 = i3 = i4 = 0;
	printf("appl_getinfo(2=AES_SYSTEM)     : %d: %d %d %d $%x\n", r, i1, i2, i3, i4);

	i1 = i2 = i3 = i4 = 0;
	r = appl_getinfo(3, &i1, &i2, &i3, &i4);
	if (r == 0) i1 = i2 = i3 = i4 = 0;
	printf("appl_getinfo(3=AES_LANGUAGE)   : %d: %d %d %d %d\n", r, i1, i2, i3, i4);

	i1 = i2 = i3 = i4 = 0;
	r = appl_getinfo(4, &i1, &i2, &i3, &i4);
	if (r == 0) i1 = i2 = i3 = i4 = 0;
	printf("appl_getinfo(4=AES_PROCESS)    : %d: %d %d %d %d\n", r, i1, i2, i3, i4);

	i1 = i2 = i3 = i4 = 0;
	r = appl_getinfo(5, &i1, &i2, &i3, &i4);
	if (r == 0) i1 = i2 = i3 = i4 = 0;
	printf("appl_getinfo(5=AES_PCGEM)      : %d: %d %d %d %d\n", r, i1, i2, i3, i4);

	i1 = i2 = i3 = i4 = 0;
	r = appl_getinfo(6, &i1, &i2, &i3, &i4);
	if (r == 0) i1 = i2 = i3 = i4 = 0;
	printf("appl_getinfo(6=AES_INQUIRE)    : %d: %d %d %d %d\n", r, i1, i2, i3, i4);

	i1 = i2 = i3 = i4 = 0;
	r = appl_getinfo(7, &i1, &i2, &i3, &i4);
	if (r == 0) i1 = i2 = i3 = i4 = 0;
	printf("appl_getinfo(7=AES_WDIALOG)    : %d: $%x %d %d %d\n", r, i1, i2, i3, i4);

	i1 = i2 = i3 = i4 = 0;
	r = appl_getinfo(8, &i1, &i2, &i3, &i4);
	if (r == 0) i1 = i2 = i3 = i4 = 0;
	printf("appl_getinfo(8=AES_MOUSE)      : %d: %d %d $%x %d\n", r, i1, i2, i3, i4);

	i1 = i2 = i3 = i4 = 0;
	r = appl_getinfo(9, &i1, &i2, &i3, &i4);
	if (r == 0) i1 = i2 = i3 = i4 = 0;
	printf("appl_getinfo(9=AES_MENU)       : %d: %d %d %d %d\n", r, i1, i2, i3, i4);

	i1 = i2 = i3 = i4 = 0;
	r = appl_getinfo(10, &i1, &i2, &i3, &i4);
	if (r == 0) i1 = i2 = i3 = i4 = 0;
	printf("appl_getinfo(10=AES_SHELL)     : %d: $%x %d %d %d\n", r, i1, i2, i3, i4);

	i1 = i2 = i3 = i4 = 0;
	r = appl_getinfo(11, &i1, &i2, &i3, &i4);
	if (r == 0) i1 = i2 = i3 = i4 = 0;
	printf("appl_getinfo(11=AES_WINDOW)    : %d: $%x $%x $%x %d\n", r, i1, i2, i3, i4);

	i1 = i2 = i3 = i4 = 0;
	r = appl_getinfo(12, &i1, &i2, &i3, &i4);
	if (r == 0) i1 = i2 = i3 = i4 = 0;
	printf("appl_getinfo(12=AES_MESSAGE)   : %d: $%x %d %d %d\n", r, i1, i2, i3, i4);

	i1 = i2 = i3 = i4 = 0;
	r = appl_getinfo(13, &i1, &i2, &i3, &i4);
	if (r == 0) i1 = i2 = i3 = i4 = 0;
	printf("appl_getinfo(13=AES_OBJECT)    : %d: %d %d %d $%x\n", r, i1, i2, i3, i4);

	i1 = i2 = i3 = i4 = 0;
	r = appl_getinfo(14, &i1, &i2, &i3, &i4);
	if (r == 0) i1 = i2 = i3 = i4 = 0;
	printf("appl_getinfo(14=AES_FORM)      : %d: %d %d %d %d\n", r, i1, i2, i3, i4);

	i1 = i2 = i3 = i4 = 0;
	r = appl_getinfo(64, &i1, &i2, &i3, &i4);
	if (r == 0) i1 = i2 = i3 = i4 = 0;
	printf("appl_getinfo(64=AES_EXTENDED)  : %d: %d %d $%x %d\n", r, i1, i2, i3, i4);

	i1 = i2 = i3 = i4 = 0;
	r = appl_getinfo(65, &i1, &i2, &i3, &i4);
	if (r == 0) i1 = i2 = i3 = i4 = 0;
	printf("appl_getinfo(65=AES_NAES)      : %d: %d %d %d %d\n", r, i1, i2, i3, i4);

	i1 = i2 = i3 = i4 = 0;
	r = appl_getinfo(96, &i1, &i2, &i3, &i4);
	if (r == 0) i1 = i2 = i3 = i4 = 0;
	printf("appl_getinfo(96=AES_VERSION)   : %d: %d %d $%x %d\n", r, i1, i2, i3, i4);

	i1 = i2 = i3 = i4 = 0;
	r = appl_getinfo(97, &i1, &i2, &i3, &i4);
	if (r == 0) i1 = i2 = i3 = i4 = 0;
	printf("appl_getinfo(97=AES_WOPTS)     : %d: $%x $%x $%x $%x\n", r, i1, i2, i3, i4);

	i1 = i2 = i3 = i4 = 0;
	r = appl_getinfo(98, &i1, &i2, &i3, &i4);
	if (r == 0) i1 = i2 = i3 = i4 = 0;
	printf("appl_getinfo(98=AES_FUNCTIONS) : %d: $%x $%x $%x $%x\n", r, i1, i2, i3, i4);

	i1 = i2 = i3 = i4 = 0;
	r = appl_getinfo(99, &i1, &i2, &i3, &i4);
	if (r == 0) i1 = i2 = i3 = i4 = 0;
	printf("appl_getinfo(99=AES_AOPTS)     : %d: $%x $%x $%x $%x\n", r, i1, i2, i3, i4);

	i1 = i2 = i3 = i4 = 0;
	r = appl_getinfo(22360, &i1, &i2, &i3, &i4);
	if (r == 0) i1 = i2 = i3 = i4 = 0;
	printf("appl_getinfo(22360=AES_WINX)   : %d: $%x $%x $%x $%x\n", r, i1, i2, i3, i4);

	i1 = i2 = i3 = i4 = 0;
	r = appl_getinfo(22358, &i1, &i2, &i3, &i4);
	if (r == 0) i1 = i2 = i3 = i4 = 0;
	printf("appl_getinfo(22358=AES_XAAES)  : %d: $%x $%x $%x $%x\n", r, i1, i2, i3, i4);

	appl_exit();
	return 0;
}
