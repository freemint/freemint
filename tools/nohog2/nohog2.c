/*
 * no_hog2.acc
 *
 * TOS AES spends much of it's time in supervisor more, preventing
 * FreeMiNT task switching when TOS AES is in use (GEM=ROM in mint.cnf).
 * This very simple accessory works around this by calling Syield every 20ms.
 *
 * Jo Even Skarstein, 2017
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 */

#include <mintbind.h>
#include <gem.h>
#include <mint/cookie.h>

#ifndef false
enum { false, true };
#endif

char alert_running[] = "[1][ NoHog2 ][ Ok ]";
char alert_no_mint[] = "[1][ NoHog2 only needed | with MiNT. ][ Ok ]";
char alert_mtask[]   = "[1][ NoHog2 only needed | with ROM AES. ][ Ok ]";

/* no need to run global constructors/destructors here */
void __main(void);
void __main(void) { }

int main(void)
{
	long c;
	short exit = false, apid, mint, mtask, e_type = MU_MESAG;
	char *alert = alert_running;

	apid = appl_init();
	menu_register(apid, "  NoHog2");

	mtask = (aes_global[1] != 1);
	mint = !Getcookie(C_MiNT, &c);

	if (!mint)
		alert = alert_no_mint;
	
	if (mint && mtask)
		alert = alert_mtask;

	// Enable 20ms timer when running under MiNT without
	// a multitasking AES.
	if (mint && !mtask)
		e_type |= MU_TIMER;

	while (!exit)
	{
		static short msg[8], mx, my, bt, ks, kb, mc;
		short e = evnt_multi( e_type,
					0, 0, 0,
					0, 0, 0, 0, 0,
					0, 0, 0, 0, 0,
					msg,
					20,
					&mx, &my, &bt, &ks, &kb, &mc);
	
		if (e & MU_MESAG)
		{
			switch (msg[0])
			{
				case AC_OPEN:
					form_alert(0, alert);
					break;
				case AP_TERM:
					exit = true;
					break;
				default:
					break;
			}
		}

		if (e & MU_TIMER)
		{
			Syield();
		}
	}

	appl_exit();
	return 0;
}
