Tera Desktop V1.41		Copyright 1991-1995  W. Klaren.
             V2.1	 	Copyright 2002       H. Robbers.
             V2.3 to V4.07  	Copyright 2003-2016  H. Robbers, Dj. Vukovic


This  is version 4.07 of the Tera Desktop, a replacement for the builtin TOS
desktop  for 16-bit and 32-bit Atari computers. This program is Freeware and
Open Source.  It is published under General Public License (GPL) which means
that it  may be  copied  and  modified freely,  providing that  the original
authorships   are   recognized  where  appropriate,  and  that  it,  or  its
derivatives, may not be sold.  See the included COPYING file for the details
on GPL.

Please note that the name  of this project is  'Tera Desktop' or 'TeraDesk',
not 'Terra Desktop'.


Package manifest:
=================

Tera Desktop binary distribution currently consists of the following files:

        COPYING       (A copy of the GPL license)
        DESKTOP.PRG   (All-environments Desktop)
        DESKTOS.PRG   (Somewhat smaller Desktop compiled for Single-TOS)
        DESK_CF.PRT   (Somewhat larger version compatible with Coldfire)
        DESKTOP.RSC   (English resource file)
        DESKTOP.RSD   (Resource object names definition file)
        ICONS.RSC     (Essential Mono icons)
        CICONS.RSC    (Essential Colour icons)
        README.TXT    (This file)
        HIST_V34.TXT  (Log of changes since Version 3.0)
        TERADESK.HYP  (English manual in ST-Guide hypertext)
        TERADESK.REF  (Reference file for the hypertext manual)
        TERADESK.INF  (Sample configuration file)
        TERADESK.PAL  (Sample palette file)

There  also  exists a source distribution which contains the complete source
tree of TeraDesk,  to be compiled and linked with Pure-C 1.1 or AHCC.  It is
located at the web homepage of TeraDesk 3 and TeraDesk 4:

	http://solair.eunet.rs/~vdjole/teradesk.htm

TeraDesk uses the  AHCM memory-allocation  system  developed  by H. Robbers. 
Included  in the source distribution of TeraDesk is the source  of  an older 
version of AHCM  with functionality sufficient to build TeraDesk.  Source of 
AHCM version newer than the one that is used in TeraDesk can be found in the
distribution of the AHCC compiler developerd by H. Robbers at: 

	http://members.ams.chello.nl/h.robbers

(the newer version of AHCM will produce slightly larger TeraDesk binary).
 
A note on the naming of distribution files:

Binary distributions have names in the form TERAnnnB.ZIP ("B" for "Binary"), 
"nnn" being the version number multipled by 100 (e.g. as "396" for 3.96).

Source-code  distributions  have  names in  the form  TERAnnnS.ZIP  ("S" for 
"Source").

Preliminary compilations sent to testers have names in the form TERAnnnP.ZIP
("P" for "Preliminary"). Contents of TERAnnnP.ZIP may vary.

It is recommended  that translated  resource and hypertext manual  files are
distributed in zip archives having names in the form TERAnnnx.ZIP, "x" being
a character conveniently marking the language of translation.



Hardware and Operating System Requirements
==========================================

Tera Desktop  can  be  used  on  any  Atari  ST  series  computer  and their
offspring,  TT,  Falcon,  Hades,  Milan or emulators.  Since version 4.04 it
has been  compatible  with Coldfire.  It uses  about 200 - 300 KB of memory,
depending on the complexity of configuration.

Although  Tera Desktop  can be used  without the aid of a hard disk, the use
of one is strongly recommended.  Tera Desktop is not  well optimized for use
on machines with  only one floppy  drive  and  no hard disk. File copying in
TeraDesk  is done  file-by-file  which,  when copying files from one disk to
another on a machine with only one drive, will mean a lot of disk swapping.

Tera Desktop  should work  with all existing versions of TOS  (i.e. starting
with  TOS 1.0) but it is much more useful with TOS versions 1.04 (also known
as TOS 1.4) and above.  However, it may fail on very old versions of Mint or
Magic (probably older than 1.12 and 3.0 respectively).

Since  Version  2.0  Tera Desktop  runs on modern multitasking environments,
such  as:  MagiC,  TOS  with Geneva, TOS with Mint and an AES (N.Aes, XaAES,
MyAES or Atari AES 4.1) etc. It can run with memory protection.

Some  features  of  Tera Desktop  may  be  nonfunctional,  depending  on the
version of the OS and the AES used.

Tera Desktop  makes  several  inquires  trying  to  determine TOS- and  AES-
versions  and  their capabilities and limitations. If some version of TOS or
AES  is not able to answer these queries, Tera Desktop tries to make guesses
which  may  not always be correct. It is also possible that incorrect answer
to  a query is supplied by TOS/AES.  In such case Tera Desktop may work with
unnecessary limitations or else try to activate features which may not work.



New Features in This Version
============================

Please, see  HIST_V34.TXT for a list of new features and bug fixes since the
last  released  version (4.06). Also, read the manual TERADESK.HYP (you will
need ST-Guide for this) for more detailed information.

Before installing any  new version of TeraDesk,  you are advised to load and
then save each  TeraDesk  configuration file that you use.  This will update
any older  versions of the  configuration file(s)  to your  current version.
TeraDesk may report errors when reading  too old configuration files. It may
also be a good idea to make a backup of the current configuration files.



Installation
============

1.  A folder named e.g. DESKTOP or TERADESK  can be created anywhere on your
floppy  or  hard  disk,  or in a RAM disk, to hold Tera Desktop files. It is
also  possible,  although  a  bit  untidy, to put Tera Desktop into the root
directory of a disk volume/partition.


2.  The following  files  should  be  copied  to the location specified  for
TeraDesk:

       DESKTOP.PRG  (if  you  intend  to use multitasking)  OR
       DESKTOS.PRG  (if you  will  work in  Single-TOS  ONLY) OR
       DESK_CF.PRG  (if you need a Coldfire-compatible version)
       DESKTOP.RSC
       ICONS.RSC    (if you will use monochrome icons) AND/OR
       CICONS.RSC   (if your AES can support colour icons)

Note  that  DESKTOP.PRG  will  work  in single-TOS as well; DESKTOS.PRG just
saves  a  few  kilobytes  of  memory  by not  containing code  which is only
relevant  in multitasking environments,  and  by  having a  somewhat limited
support of the AV-protocol  (AV-protocol functions  not likely to be used in
Single-TOS are removed). Beware that the single-TOS version may not properly
interpret some configuration files created in the multitasking version, e.g.
if they contain any references to symbolic links or long filenames.

The Coldfire version should be compatible with  68020 and  higher processors
as well.

If  only  DESKTOS.PRG  is to be used, it may be renamed to DESKTOP.PRG after
copying,  but this is  not required;  the program will  regiser itself  with
the AES as "DESKTOP" anyway. 

On a Coldfire system,  one should use  DESK_CF.PRG  instead of  DESKTOP.PRG. 
Same as with  DESKTOS.PRG  the file can be  renamed to  DESKTOP.PRG but this 
is not required.

One  can  (but  need  not) also copy into this folder the files TERADESK.INF
and  TERADESK.PAL  from  the  \EXAMPLES  folder.  Note,  however, that these
example configuration files  are set for one hypothetical configuration, and
may  not  be  appropriate  for your setup (Tera Desktop will attempt to obey
everything  specified  in the configuration files, no matter what the actual
environment is).

If you start Tera Desktop without TERADESK.INF in its directory, the program
will complain that it can not find its configuration file. In order to avoid
this message  appearing again, activate "Save settings" in the Options menu.
Of course,  before saving,  the configuration can first be adjusted  to  any
user's  particular environment and taste  by defining additional desktop and
window icons, filetypes, etc.


3.  If you use ST-GUIDE  or other compatible  hypertext viewer  application,
copy TERADESK.HYP and TERADESK.REF from the \DOC  folder to the folder where
your other .HYP files are.  When you first start Tera Desktop,  install your
hypertext viewer  (program or accessory)  as the  application for the  *.HYP
filetype and save the configuration.  The hypertext manual for TeraDesk will
thereafter be available upon [Shift][Help] keypress.


4.  Cooperation  of  Tera Desktop  with  some  other  applications  will  be
improved  if it is announced that certain protocols can be handled. In order
to do so, the following environment variables can be defined:

 AVSERVER=DESKTOP
 FONTSELECT=DESKTOP

These  declare  TeraDesk  as  the  AV-Server  and  as  the font-selector. In
Single-TOS  configuration  these  protocols  can be used by some accessories
(such  as  ST-Guide);  in  multitasking configurations they can be used by a
much larger number of concurrently running applications.

The manner of declaration  of environmental variables depends on variants of
the OS and utilities used.


5.  It is convenient to set TeraDesk to start automatically at system boot.

If you use  (Single)  TOS version 1.04  (also known as TOS 1.4)  or greater,
you can  set it up by installing it  as an application, and then setting its
boot status to 'Auto' (remember to save this desktop configuration). As from
now on  Tera Desktop will take over  all  desktop tasks, prior to saving the
desktop  configuration  all  other  applications  should be deinstalled, all
unneeded  icons removed from the desktop and all windows closed. This is not
required  but  will reduce the size of DESKTOP.INF (or NEWDESK.INF) and will
also free some memory.

If you have  TOS version 1.0 or 1.02 you must use a program such as STARTGEM
to run DESKTOP from an AUTO folder.

If  you  use  Atari  AES 4.1, you can put something similar to the following
directive into your GEM.CNF file:

     shell path\to\TeraDesk\desktop.prg

Then,  the  built-in  desktop  of  AES 4.1 will not be loaded at startup and
TeraDesk will run as the desktop instead.

If  you use  Geneva,  N.AES,  XaAES,  or  MyAES  you should in a similar way
specify   TeraDesk   as   the   shell  in  the  appropriate  places  in  the
configuration  files  of  these AESes (i.e. in GEM.CNF, N_AES.CNF, XAAES.CNF
and MYAES.CNF, respectively).

If  you  use  MagiC,  TeraDesk  should be specified as a shell via the #_SHL
directive in MAGX.INF:

     #_SHL path\to\TeraDesk\desktop.prg


6.  All text strings used by Tera Desktop are located in DESKTOP.RSC  except
default filenames  and  a warning  that DESKTOP.RSC can not be found.  It is
possible  to  completely  adapt  TeraDesk  to  other  languages  by  using a
translated  DESKTOP.RSC  and, possibly, ICONS.RSC and CICONS.RSC, if someone
is willing to supply it/them. In the source distribution there exists a file
named RESOURCE.TXT containing some comments that may be of use to people who
wish to make such translations.


7.  The icon files supplied  contain a basic  set of  icons only.  Users are
encouraged  to create  thier own,  more extensive  icon files;  they can add
icons  at will,  or use other  icon files (e.g. one can rename  DESKICON.RSC
and/or  DESKCICN.RSC  used by the built-in desktop of TOS 2/3/4 to ICONS.RSC
and  CICONS.RSC respectively, and use them with TeraDesk, but any files used
should contain some icons which are essential to TeraDesk- see below).

Tera Desktop,  since  V2.0,  handles icons by name, not by index. Icons with
the following names  (or  their  translated  equivalents)  should  always be
present in the icons resource file as defaults:

FLOPPY, HARD DISC, TRASH, PRINTER, FOLDER, FILE, APP

If  any icon  name  can  not  be  found in the icons resource file, TeraDesk
attempts  to  use  one  of the default icons, according to item type. If the
default  icon  does  not  exist either, the first icon in the icons resource
file is used.

To facilitate  adaptation of TeraDesk to other languages, names of the seven
essential  icons  are  not  hard-coded  but are read from DESKTOP.RSC. It is
possible,  by editing this file, to change the names by which the icons will
be searched for in the icon resource file(s).


8.  When used  in an environment  which is supposed to support colour icons,
TeraDesk  tries  to load  the colour icon file CICONS.RSC.  If this file can
not be found, TeraDesk falls back to monochrome icons file ICONS.RSC.

Some  versions  of  AES  (e.g.  Geneva  4)  declare themselves as capable of
handling colour icons,  but that fails to work with TeraDesk. In such cases,
CICONS.RSC  file  should   be  removed,  and  TeraDesk  will  fall  back  to
monochrome   icons.   Colour  icons  file  can  also  be  removed  in  other
environments if there is a need to preserve as much free memory as possible.

9.  Users are advised to load,  check and save  any existing  Tera Desktop's
configuration files.  As the  format  of  configuration  files  may   change
slightly with new releases,  by this procedure  they will be kept up to date
as much as possible.


Read the hypertext manual for more information.



Some Possible Future Developments
=================================

Here follows a list  of ideas  that  were considered  as  likely  courses of
further development  of  TeraDesk.  Unfortunately,  as time   passes and the
author/maintainer  is  not  getting  any  younger,  it becomes less and less
likely that they will be implemented.

- Further  optimization  of  code to reduce size and memory use and increase
  speed.

- Use of  advanced features of the AHCM package  to further improve handling
  of memory allocation when  large blocks  are allocated  (e.g. when reading
  files or opening flying dialogs).

- Improvement of the algorithm for file copying when floppies are involved.
  (this has become practically irelevant as floppies have disappeared).

- Removal of the 2 GB limitation for manageable folder sizes

- Complete compliance to the  AV-protocol and Drag & Drop protocol  (some of
  rarely used AV-protocol capabilities are currently unsupported).

- Better implementation of memory-limit and no-multitask options.

- Capability to change video mode in AESes earlier than V4.

- Capability to shutdown Aranym from single-TOS.

- Capability to show a 'tree view' directory window of all drives or maybe
  to show any directory window with 'tree view'.

- Integration of a non-modal, windowed, long-names-capable fileselector  by
  producing a special directory window with a menu bar. TeraDesk's already-
  existing capabilities for selecting and  sorting directory items would be
  used,  so  a duplication of effort  and memory waste  that exists  when a 
  separate file-selector is used would be avoided.  File-selector should be 
  Selectric compatible.  AV-protocol capabilities of  TeraDesk  may be used 
  (or abused) here  and a   small  auxilliary  accessory  may be needed for
  operation in Single-TOS.

- Integration of a windowed command-line interpreter that will use the code
  already existing in TeraDesk for most (or all) of its functions.

- Capability to run TOS programs in a window in single-TOS.

- Capability to define more than one printer, on different interfaces, or to
  use GDOS/NVDI printing devices.



Some Very Unlikely To Happen Future Developments
================================================

- There are  no plans  to handle desktop background pictures in TeraDesk. It
  would be a completely nonfunctional feature, which would increase  program
  size unacceptably.  Besides, it is the opinion  of the  current maintainer
  that background pictures are in fact distracting  to  the user and a waste
  of system  resources.  However,  MyAES,  XaAES,  as well as  D.Mequignon's
  utility named PICDESK provide this feature for (almost) any desktop. 
  The hypertext manual  for TeraDesk  explains   how TeraDesk  should be set
  to use the background picture provided by an AES or other agent.



Comments and Bug-Reports
========================

It  will  be  appreciated,  if  problems   are  reported   with  a  complete
description  of  the  problem  and the configuration you are using (machine,
TOS-version,  autoboot  programs, accessories etc.). Mention TeraDesk in the
subject line of your e-mail.

THE AUTHORS OF  TERA DESKTOP  CAN NOT BE  HELD RESPONSIBLE  for  any form of
damage  caused  by this  program  or any  of  its components;  usage  of all 
components of TeraDesk is at your own risk.  See also  the accompanying file
COPYING for the terms of the General Public License.

PLEASE  read the manual and  the development-history file before you use the
program. You will need ST-Guide  (not supplied with Tera Desktop) or another
.HYP file viewer to read the manual.

Comments should be sent to: vdjole@EUnet.rs

If  you  intend to use TeraDesk, it will be appreciated if you send an e-mail
to the above address; I may at some time ask a question or two 
about TeraDesk's behaviour.


                                            Djordje Vukovic
                                            Beograd; December 7th 2013


