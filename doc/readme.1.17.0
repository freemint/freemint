==============================================================================
 ==                                                                        ==
    ==                     FreeMiNT 1.17.0 release                      ==
    ==              Dedicated to the memory of Frank Naumann            ==
 ==                                                                        ==
==============================================================================


It has been a very long time since the last official FreeMiNT kernel release.
1.16 was only ever released in a BETA form and the 1.15 release dates back over
10 years. The new 1.17.0 release is a culmination of 1.16 over the years with
some notable new items and bug fixes.

One thing that this release really lacks is Frank Naumann. Frank passed
away on the 12th March 2010. He will be sorely missed from the Atari community
and this release is dedicated to him and the efforts he made during his life.

Alan Hourihane <alanh@freemint.org>
------------------------------------
December 29th, 2010


XAAES Changes:
--------------

- Updated documentation
- Improved Task manager
  ( Detailed process info, also list application-windows, Load Graph )
- Single Tasking mode
- File Selector: improved case sensitive pattern matching,
  if the first char in pattern is the pipe-sysmbol (|) the
  pattern-list is caseinsensitive,
  concatenation of patterns with |
- launchpath config variable (previously "launcher"):
  Extended pattern matching, example:
  launchpath=<somepath>\*.prg|*.app|*.tos|*.ttp|*.acc|!*.*
- new config-variable: snapshot (can lunch internal & external snapshot app)
- Adjustable default font size (config-variable standard_font_point)
  for each app (app_options).
- New global config-variable "infoline_point" for size of window-infoline
- Support for "extended textures"
- Improved compatibility with EmuTos
- Manages a list of processes which shouldn't be killed shutdown
 (CTLALTA_SURVIVORS)
- Implement text-effects for list-text:
   Italic:<i>text</i>
   Bold:<b>text</b>
   Underlined:<u>text</u>
- New ctrl-alt key-shortcuts (look into XaAES about Dialog for a list)
- It is now possible to run XaAES from atari-desktop.
- Content of about-window is read at runtime from xa_help.txt.

XAAES Bugfixes:
---------------
 - Fixed many redraw issues ( like drawing of transparent RSC Elements )
 - Several bugfixes for Resource File loading
 - Various other Bugfixes of "small bugs" ( not so interesting for users )
 - Corrected the font-selector: List all fonts, style and family only 
   valid for outline-fonts.

Kernel Changes:
---------------

 - FAT fs: Renaming of files in-use is allowed
 - Single Tasking mode

Kernel Bugfixes:
----------------

 - fixed load of keyboard tables when memory protection is turned on
 - Shutdown procedure fixed
 - TCP connect sequence fixed
 - Various other Bugfixes of "small bugs" ( not so interesting for users )
