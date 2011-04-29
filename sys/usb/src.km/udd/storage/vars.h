/*
 * Modified for the FreeMiNT USB subsystem by David Galvez. 2010 - 2011
 *
 * TOS 4.04 Xbios vars for the CT60 board
 * Didier Mequignon 2002-2005, e-mail: aniplay@wanadoo.fr
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef	_VARS_H
#define	_VARS_H


#define etv_timer   0x400
#define etv_critic  0x404
#define memvalid    0x420
#define memctrl     0x424
#define resvalid    0x426
#define resvector   0x42A
#define phystop     0x42E
#define _memtop     0x436
#define memval2     0x43A
#define flock       0x43E
#define _timer_ms   0x442
#define _bootdev    0x446
#define sshiftmd    0x44C
#define _v_bas_ad   0x44E
#define vblsem      0x452
#define nvbls       0x454
#define _vblqueue   0x456
#define colorptr    0x45A
#define _vbclock    0x462
#define _frclock    0x466
#define hdv_init    0x46A
#define HDV_BPB     0x472
#define hdv_bpb    0x472
#define HDV_RW      0x476
#define hdv_rw     0x476
#define hdv_boot    0x47A
#define HDV_MEDIACH 0x47E
#define hdv_mediach 0x47E
#define _cmdload    0x482
#define conterm     0x484
#define trp14ret    0x486
#define __md        0x49E
#define savptr      0x4A2
#define _nflops     0x4A6
#define con_state   0x4A8
#define save_row    0x4AC
#define _hz_200     0x4BA
#define _DRVBITS    0x4C2
#define _drvbits    0x4C2
#define DSKBUFP     0x4C6
#define _dskbufp    0x4C6
#define _dumpflg    0x4EE
#define _sysbase    0x4F2
#define exec_os     0x4FE
#define dump_vec    0x502
#define ptr_stat    0x506
#define ptr_vec     0x50A
#define aux_sta     0x50E
#define aux_vec     0x512
#define PUN_PTR     0x516
#define memval3     0x51A
#define proc_type   0x59E
#define COOKIE      0x5A0
#define cookie	    0x5A0
                  
#endif
