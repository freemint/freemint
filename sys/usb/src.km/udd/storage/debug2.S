/*  
 * Modified for the FreeMiNT USB subsystem by David Galvez. 2010-2012
 *
 * Debug the CT60
 * 
 * Didier Mequignon, 2003-2006, e-mail: aniplay@wanadoo.fr
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

	.global _bconout_from_S

#include "vars.h"

	.global display_string,hex_long,hex_word,hex_byte,display_char,wait_key,_debug

	.text
	
hex_long:
	move.l D0,-(SP)
	swap D0
	bsr.s hex_word
	move.l (SP)+,D0
hex_word:
	move.w D0,-(SP)
	lsr.l #8,D0          
	bsr.s hex_byte     
	move.w (SP)+,D0
hex_byte:
	move.w D0,-(SP)
	lsr.l #4,D0        
	bsr.s hex_char      
	move.w (SP)+,D0      
hex_char:
	and.l #0xF,D0      
	or.l #0x30,D0      
	cmp.b #0x3A,D0     
	bcs.s display_char  
	addq.l #7,D0   
	    
display_char:
	and.l #0xFF,D0  
	lea -24(SP),SP
	movem.l D0-D2/A0-A2,(SP)
	move.w D0,-(sp)
	jsr _bconout_from_S       // Bconout
	addq.l #2,SP
	movem.l (SP),D0-D2/A0-A2
	lea 24(SP),SP
	rts
	
display_string:
	lea -8(SP),SP
	movem.l D0/A0,(SP)
.os2:
	move.b (A0)+,D0
	beq.s .os1
	bsr display_char
	bra.s .os2
.os1:
	movem.l (SP),D0/A0
	lea 8(SP),SP
	rts
