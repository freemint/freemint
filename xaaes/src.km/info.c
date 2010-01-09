/*
 * Init certain things in the info_tab used by appl_getinfo()
 *
 * In non-release-version build status-string
 *
 * file is updated on every build
 *
 */

#include "xa_global.h"
#include "info.h"
#include "version.h"
#include "xversion.h"

extern short info_tab[][4];	/* xa_appl.c */

#if !XAAES_RELEASE
static char *mcs(char *d, char *s)
{
	while ((*d++ = *s++))
		;

	return d - 1;
}
#endif

void
init_apgi_infotab(void)
{
#if !XAAES_RELEASE
	char *s = info_string;
	

	/*
	 * Build status string
	 */

#if 0
	s = mcs(s, version);
	*s++ = 0x7c;
	

	if (DEV_STATUS & AES_FDEVSTATUS_STABLE)
		s = mcs(s, "Stable ");
	else
		s = mcs(s, "");
		
	s = mcs(s, ASCII_DEV_STATUS);
	*s++ = 0x7c;
	s = mcs(s, ASCII_ARCH_TARGET);
	*s++ = 0x7c;
#endif

	s = mcs(s, BDATETIME);
	/*
	s = mcs(s, BDATE);*s++ = 0x20;
	s = mcs(s, BTIME);
	*/
	*s++ = 0x7c;
	s = mcs(s, BCOMPILER);
#if 0
	s = mcs(s, BC_MAJ);
	s = mcs(s, ".");
	s = mcs(s, BC_MIN);
#endif
	*s++ = 0;

	DIAGS(("Build status-string '%s'", info_string));
#endif
	info_tab[0][0] = screen.standard_font_height;
 	info_tab[0][1] = screen.standard_font_id;
	info_tab[1][0] = screen.small_font_height;
	info_tab[1][1] = screen.small_font_id;
	
	info_tab[2][0] = xbios_getrez();
	info_tab[2][1] = 256;
	info_tab[2][2] = 1;
	info_tab[2][3] = 1; // + 2;
}
