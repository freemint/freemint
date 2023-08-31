#include "mint/asmdefs.h"

    .globl	SYM(usbxhdi)
    .extern	SYM(xhdi_handler)

    dc.l	0x27011992	// XHDIMAGIC
SYM(usbxhdi):
    // Important: The XHDI specification requires to backup all the registers
#ifdef __mcoldfire__
    lea     -12(sp),sp
    movem.l d1/a0-a1,(sp)
#else
    movem.l d1/a0-a1,-(sp)
#endif
    pea     16(sp)
    jsr     SYM(xhdi_handler)
    addq.l  #4,sp
#ifdef __mcoldfire__
    movem.l (sp),d1/a0-a1
    lea     12(sp),sp
#else
    movem.l (sp)+,d1/a0-a1
#endif
    rts
