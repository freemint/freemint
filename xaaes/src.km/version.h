/*
 * $Id$
 *
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann & O.Skancke
 *
 * A multitasking AES replacement for FreeMiNT
 *
 * This file is part of XaAES.
 *
 * XaAES is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * XaAES is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _xaaes_version_h
#define _xaaes_version_h

#define DSMASM 0x00ff

/*
 * main version macros
  */
#define XAAES_MAJ_VERSION  1
#define XAAES_MIN_VERSION  5
#define XAAES_PATCH_LEVEL  5

/* set either of these */
//#define DEV_STATUS	AES_DEVSTATUS_BETA
#define DEV_STATUS	AES_DEVSTATUS_RELEASE

/* This is only used in wind_get(WF_XAAES) which will be removed */
#define HEX_VERSION	0x09

#define SHORT_NAME		"XaAES"
#define AES_ID		"   " SHORT_NAME

#define LONG_NAME	"XaAES Ain't the AES, a free MultiTasking AES for FreeMiNT"

#ifndef AES_DEVSTATUS_ALPHA
/* appl_getinfo(AES_VERSION) return values */
#define AES_DEVSTATUS_ALPHA	0
#define AES_DEVSTATUS_BETA	1
#define AES_DEVSTATUS_RELEASE	2
#define AES_FDEVSTATUS_STABLE	0x100
#endif

#if ((DEV_STATUS & 0x000f) == AES_DEVSTATUS_ALPHA)
#define ASCII_DEV_STATUS	"Alpha"
#endif

#if ((DEV_STATUS & 0x000f) == AES_DEVSTATUS_BETA)
#ifdef XAAES_RELEASE
#undef XAAES_RELEASE
#endif
#define XAAES_RELEASE		0
#define ASCII_DEV_STATUS	"Beta"
#endif

#if ((DEV_STATUS & 0x000f) == AES_DEVSTATUS_RELEASE)
#ifdef XAAES_RELEASE
#undef XAAES_RELEASE
#endif
#define XAAES_RELEASE 		1
#define ASCII_DEV_STATUS	"Release"
#endif

#undef ARCH_TARGET

#ifndef ASCII_DEV_STATUS
#define ASCII_DEV_STATUS	"Undefined"
#endif

#ifdef __mcoldfire__
 #define _CPU	"coldfire"
 #define ARCH_TARGET	AES_ARCH_COLDFILRE
#else
 #ifdef mc68060
  #ifdef mc68020
   #define _CPU	"m68020-060"
   #define ARCH_TARGET	AES_ARCH_M6802060
  #else
   #define _CPU	"m68060"
   #define ARCH_TARGET	AES_ARCH_M68060
  #endif
 #else
  #ifdef mc68040
   #define _CPU	"m68040"
   #define ARCH_TARGET	AES_ARCH_M68040
  #else
   #ifdef mc68030
    #define _CPU	"m68030"
    #define ARCH_TARGET	AES_ARCH_M68030
   #else
    #ifdef mc68020
     #define _CPU	"m68020"
     #define ARCH_TARGET	AES_ARCH_M68020
    #else
     #ifdef mc68010
      #define _CPU	"m68010"
      #define ARCH_TARGET	AES_ARCH_M68010
     #else
      #define _CPU	"m68000"
      #define ARCH_TARGET	AES_ARCH_M68000
     #endif
    #endif
   #endif
  #endif
 #endif
#endif



#ifdef ARCH_TARGET
#define ASCII_ARCH_TARGET	_CPU
#else
#define ASCII_ARCH_TARGET	"Undefined"
#endif


#define BDATE		__DATE__
#define BTIME		__TIME__
#define BC_MAJ		str (__GNUC__)
#define BC_MIN 		str (__GNUC_MINOR__)
#define BCOMPILER	str (__GNUC__) "." str (__GNUC_MINOR__)
//"gcc 2.95.3"

// const char BCOMPILER [] = str (__GNUC__) "." str (__GNUC_MINOR__);

#endif /* _xaaes_version_h */
