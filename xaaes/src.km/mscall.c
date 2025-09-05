/*
 * This file contains real functions to substitute the inlined versions
 */
#include "xa_global.h"

static int MsCnt = 0;

void hidem(void)
{
	v_hide_c(C.P_handle);
	MsCnt++;
}

void showm(void)
{
	if (MsCnt > 0)
	{
		v_show_c(C.P_handle, 1);
		MsCnt--;
	}
}

void forcem(void)
{
	if (MsCnt > 0)
		v_show_c(C.P_handle, 0);
	MsCnt = 0;
}

int tellm(void)
{
	return MsCnt;
}
