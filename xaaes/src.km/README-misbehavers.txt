 ------------------------------------------------------------------------------

 Author: Odd Skancke, started 09 May 2004, 01:00 at night, very tired.

------------------------------------------------------------------------------

 This file is a list of AES applications found to misbehave, more or less 
 badly, according to AES and GEM programming guidelines. The first rule of any 
 program wishing to use the AES is that it must call appl_init() before using 
 _ANY_ AES functions. The following is a list of detected 'misbehavers';

------------------------------------------------------------------------------
 Porthos 3.24 / 11.06.2003 - "Not registered version"

 This version of Porthos calls the following AES functions upon startup;

 graf_mouse()
 appl_init()
 .....
 appl_exit()

 This is wrong! It calls graf_mouse() before appl_init(), which is a no-no.

------------------------------------------------------------------------------
 imgc4cd (Imagecopy 4.01b6 CD  - 15 December 1996)
 ic4plus (Imagecopy 4.2b2 Plus -  3 April 1998)

 Both of these calls appl_init() followed immediately by a appl_exit() call. 
 This is extremely weird, and it is nothing short of incredible that this have 
 worked on other AES all this time :-) XaAES is too clean to allow such 
 behaviour normally, so a check for these two applications is done, and 
 appl_exit() calls are ignored for them. Upon exiting, they both call 
 appl_exit() a second time, and this time it is correct.

 In addition, ic4plus calls the AES with a opcode 12042, which is either a big 
 fat bug, or something I have no clue about.

------------------------------------------------------------------------------
 Tempus Word NG 5.30 - 08.04.2004 "Demo version"

 This application does not work with MP enabled. Hopefully this will be fixed.

------------------------------------------------------------------------------
 HighWire (my own build of the CVS sourcetree on 8th of May, 2004 at noon)

 This is a fantastic piece of software! However, the version here calls 
 graf_handle() as its first AES function, before it calls appl_init(). 
 Hopefully this problem will go away when I speak to Ralph ;-)

------------------------------------------------------------------------------
 Thingfnd Version 0.10 (Jun 24 1999)

  This utility for Thing Desktop does the following sequence of AES calls. 
Thomas Binder, you know better than this? ;-)

 graf_hand()
 appl_find()
 appl_getinfo()
 appl_init()

------------------------------------------------------------------------------
 Jinnee Versions prior 2.5

 It has the value of -1 (0xffffffff) by each CICON blk resolution list instead
 of valid NULL value.

 Interesting thing is that it uses (I haven't met another app) legal(?)
 requirement that the resolution fixed CICON bitmap planes field should be
 updated to reflect the bitmap acutal format.

------------------------------------------------------------------------------
 Papyrus Version 8.23A "Demo version of 28.7.2000"

 shel_read()
 appl_init()
------------------------------------------------------------------------------

 MyMail version 1.58-62 6696

 This one does many weird things. The thing I (ozk) noticed when trying to 
debug it was that it enters the AES with a NULL pb pointer when I click on the 
mailbox icon to check for new mail. I have no idea why, but it is promptly 
killed by XaAES because of it.

 Weird as he.. ehm .. weird! When testing and reporting the above, I was using 
 Nova VDI with NVDI 5.01. Just now I tested with oVDI, and MyMail did not have 
 this problem! Hmmm... If anyone tests MyMail I'd be interested in feedback!

------------------------------------------------------------------------------
 gewicth
 
  Gewicht seems to check an undocumented variable in aes_global structure for
the size of the loaded resource after a rsrc_load() call instead of correctly
checking returnvalue of rsrc_load(). This element being an undocumented one,
XaAES didn't correctly fill it, and Gewicth reported it could not find its
resource file.


