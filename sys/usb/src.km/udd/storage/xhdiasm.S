	.text

	.globl	_usbxhdi
	.globl	_xhdi_handler

	dc.l	0x27011992	// XHDIMAGIC
_usbxhdi:
	jmp	_xhdi_handler
