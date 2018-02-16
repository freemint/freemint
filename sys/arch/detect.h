/* 
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Author: Jrg Westheide <joerg_westheide@su.maus.de>
 * Started: 1998-12-20
 * 
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 */

# ifndef _m68k_detect_h
# define _m68k_detect_h

extern void *ControlRegA, *DataRegA;
extern void *ControlRegB, *DataRegB;

long detect_hardware (void);

long detect_cpu (void);
long detect_fpu (void);
short detect_pmmu (void);

long _cdecl test_byte_rd(long addr);
long _cdecl test_word_rd(long addr);
long _cdecl test_long_rd(long addr);

# endif  /* _m68k_detect_h */
