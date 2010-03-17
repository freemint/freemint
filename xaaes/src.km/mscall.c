/*
 * This file contains real functions to substitute the inlined versions
 */
#include "xa_global.h"

#if !AGGRESSIVE_INLINING
void hidem(void){  v_hide_c(C.P_handle);	}
void showm(void){ v_show_c(C.P_handle, 1);	}
void forcem(void){ v_show_c(C.P_handle, 0);	}
#endif
