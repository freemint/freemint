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

#include <stdlib.h>
#include <string.h>
#include "diallib.h"
#include "defs.h"

void LoadConfig(void)
{
	extern char cfg_comment;
	if(CfgOpenFile(PROGRAM_UNAME".CFG"))
	{
		char *variable;

		cfg_comment = '#';

		variable = CfgGetVariable("PATH=");
		if(*variable)
			strncpy(path_list,variable,512);
		variable=CfgGetVariable("DEFAULT=");
		if(*variable)
			strncpy(default_file,variable,DL_PATHMAX);
		variable=CfgGetVariable("KATALOG=");
		if(*variable)
			strncpy(catalog_file,variable,DL_PATHMAX);

		variable=CfgGetVariable("WINPOS=");
		if(*variable)
		{
			win_x=atoi(CfgExtractVarPart(variable,',',0));
			win_y=atoi(CfgExtractVarPart(variable,',',1));
			win_w=atoi(CfgExtractVarPart(variable,',',2));
			win_h=atoi(CfgExtractVarPart(variable,',',3));
		}
		variable = CfgGetVariable("WINADJUST=");
		if (*variable)
			adjust_winsize = atoi(variable);

		variable=CfgGetVariable("FONT=");
		if(*variable)
		{
			font_id=atoi(CfgExtractVarPart(variable,',',0));
			font_pt=atoi(CfgExtractVarPart(variable,',',1));
		}
		variable=CfgGetVariable("XFONT=");
		if(*variable)
		{
			xfont_id=atoi(CfgExtractVarPart(variable,',',0));
			xfont_pt=atoi(CfgExtractVarPart(variable,',',1));
		}
		
		variable=CfgGetVariable("BGRND_COLOR=");
		if(*variable && ext_workout[4]>=4)	/*	>= 16 Farben?	*/
			background_col=atoi(variable);

		variable=CfgGetVariable("TEXT_COLOR=");
		if(*variable && ext_workout[4]>=4)	/*	>= 16 Farben?	*/
			text_col=atoi(variable);

		variable=CfgGetVariable("LINK_COLOR=");
		if(*variable && ext_workout[4]>=4)	/*	>= 16 Farben?	*/
			link_col=atoi(variable);

		variable=CfgGetVariable("LINK_EFFECT=");
		if(*variable)
			link_effect=atoi(variable);

		variable=CfgGetVariable("TRANSPARENT_PICS=");
		if(*variable)
			transparent_pics=atoi(variable);

		variable=CfgGetVariable("BIN_COLUMNS=");
		if(*variable)
			binary_columns=atoi(variable);

		variable=CfgGetVariable("ASCII_TAB=");
		if(*variable)
			ascii_tab_size=atoi(variable);

		variable=CfgGetVariable("VA_START_NEWWIN=");
		if(*variable)
			va_start_newwin=atoi(variable);
		
		variable=CfgGetVariable("DEBUG_FILE=");
		if(*variable)
			strncpy(debug_file,variable,DL_NAMEMAX);

		variable=CfgGetVariable("CHECK_TIME=");
		if(*variable)
			check_time=atoi(variable);

		variable=CfgGetVariable("SKIN=");
		if(*variable)
		{
		char *ptr=skin_path;
			/*	relative path given?	*/
			if(variable[1] != ':')
			{
				while(*ptr)
					ptr++;
				*ptr++=dir_separator;
				
			}
			strcpy(ptr,variable);
		}
		else
			*skin_path=0;
			
		variable=CfgGetVariable("ASCII_BREAK=");
		if(*variable)
		{
			ascii_break_len=max(0,atoi(variable));
			ascii_break_len=min(LINE_BUF-1,ascii_break_len);
		}

		variable=CfgGetVariable("INTELLIGENT_FULLER=");
		if(*variable)
			intelligent_fuller=atoi(variable);
		
		variable=CfgGetVariable("CLIPBRD_NEW_WINDOW=");
		if(*variable)
			clipbrd_new_window=atoi(variable);
		
		variable=CfgGetVariable("AV_WINDOW_CYCLE=");
		if(*variable)
			av_window_cycle=atoi(variable);
		
		variable=CfgGetVariable("MARKFILE=");
		if(*variable)
			strncpy(marken_path,variable,DL_PATHMAX);

		variable=CfgGetVariable("MARKFILE_SAVE_ASK=");
		if(*variable)
			marken_save_ask=atoi(variable);

		variable=CfgGetVariable("REF=");
		if(*variable)
			strncpy(all_ref,variable,DL_PATHMAX);

		variable=CfgGetVariable("HYPFIND=");
		if(*variable)
			strncpy(hypfind_path,variable,DL_PATHMAX);

		variable=CfgGetVariable("REFONLY=");
		if(*variable)
			refonly = TRUE;

		CfgCloseFile();
	}
	else
	{
		*skin_path=0;
	}
}

