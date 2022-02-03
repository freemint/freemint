/*
 * lpflush.c: flushes the "/dev/lp" buffer. Very useful when you want
 * to stop a catastrophy without rebooting the computer
 */
 
#include <mint/mintbind.h>
#include <mint/dcntl.h>

#define DEVNAME "u:\\dev\\lp"

int
main(void)
{
	int fp, ret;
	
	fp = Fopen(DEVNAME, 0);
	if (fp < 0)
	{
		(void) Cconws("lpflush: can't open \"" DEVNAME "\"\r\n");
		return fp;
	}
	
	ret = Fcntl(fp, 0L, TIOCFLUSH);
	if (ret < 0)
	{
		(void) Cconws("lpflush: can't flush \"" DEVNAME "\"\r\n");
		return ret;
	}
	
	return 0;  /* Ok */
}
