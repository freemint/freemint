/*
 * HypView - (c)      - 2006 Philipp Donze
 *               2006 -      Philipp Donze & Odd Skancke
 *
 * A replacement hypertext viewer
 *
 * This file is part of HypView.
 *
 * HypView is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * HypView is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HypView; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <gem.h>
#include "diallib.h"
#include "defs.h"

extern short _app;

int
main(int argc, char *argv[])
{
	if(DoAesInit())
		return(0);

	if(DoInitSystem())
		return(0);

	LoadConfig();						/*	Konfiguration laden	*/

	Init();								/*	restliche Initialisierung	*/

	if(!_app)							/*	Als ACC gestartet?	*/
		menu_register(ap_id,"  " PROGRAM_NAME);	/*	...im Menu anmelden	*/

	if(argc <= 1)							/*	Keine Parameter bergeben?	*/
	{
		if(_app)							/*	Als Programm gestartet?	*/
		{
			if (*default_file)			/*	Default-Hypertext angegeben?	*/
				OpenFileNW(default_file,NULL,0);
			else
				SelectFileLoad();		/*	Datei per Fileselector erfragen	*/
		}
	}
	else									/*	Falls Parameter angegeben...	*/
	{
		/*	...diese Datei (inkl. Kapitel) laden	*/
		OpenFileNW(argv[1], (argc > 2 ? argv[2]:NULL), 0);
	}

	while(!_app || (!doneFlag && all_list))
	{
		DoEvent();
		if(quitApp)
			break;
	}
	
	RemoveItems();

	Exit();

	DoExitSystem();
	return(0);
}
