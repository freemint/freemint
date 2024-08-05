#!/bin/bash -eux
# -e: Exit immediately if a command exits with a non-zero status.
# -u: Treat unset variables as an error when substituting.
# -x: Display expanded script commands

SRC="$1"
DST="$2"
VER="$3"
CUR="$4"

. "$SCRIPT_DIR/util.sh"
. "$SCRIPT_DIR/kernel_targets.sh"

GUIDESDIR="$DST/guides"
AUTODIR="$DST/auto"
MINTDIR="$DST/mint/$VER"
XAAESDIR="$MINTDIR/xaaes"
SYSROOT="$MINTDIR/sys-root"
USBDIR="$MINTDIR/usb"
TBLDIR="$MINTDIR/keyboard"
FONTSDIR="$MINTDIR/fonts"

# TODO: mintloader doesn't detect Hatari (but it could via trap #1 handler pointing to the cartridge area)
if [ -n "${VERSIONED+x}" ]
then
	copy_auto "$AUTODIR" "$CPU_TARGET" "-$CUR"
	copy_kernel "$MINTDIR" "$KERNEL_TARGETS"
	copy_mint_cnf "$MINTDIR"
else
	copy_auto "$AUTODIR" "$CPU_TARGET" "load"
	copy_kernel "$MINTDIR" "$KERNEL_TARGETS"
fi
copy_modules "$MINTDIR" "$CPU_TARGET"

# mchdir: st, ste, megaste, tt, falcon, milan, hades, ct60, firebee, aranym
if [ "$CPU_TARGET" != "col" ]
then
	if [ "$CPU_TARGET" != "deb" ]
	then
		copy_m68k_modules "$MINTDIR" "$CPU_TARGET"
	fi
	if [ "$CPU_TARGET" = "000" ]
	then
		copy_st_modules "$MINTDIR"
		# TODO: this one is not detected by FreeMiNT !!!
		copy_megast_modules "$MINTDIR"
		copy_ste_modules "$MINTDIR"
		copy_megaste_modules "$MINTDIR"
	elif [ "$CPU_TARGET" = "02060" ]
	then
		copy_tt_modules "$MINTDIR"
		copy_falcon_modules "$MINTDIR"
		copy_milan_modules "$MINTDIR"
		copy_hades_modules "$MINTDIR"
		copy_ct60_modules "$MINTDIR"
		copy_aranym_modules "$MINTDIR"
	elif [ "$CPU_TARGET" = "030" ]
	then
		copy_tt_modules "$MINTDIR"
		copy_falcon_modules "$MINTDIR"
	elif [ "$CPU_TARGET" = "040" ]
	then
		copy_falcon_modules "$MINTDIR"
		copy_milan_modules "$MINTDIR"
		copy_hades_modules "$MINTDIR"
		copy_aranym_modules "$MINTDIR"
	elif [ "$CPU_TARGET" = "060" ]
	then
		copy_milan_modules "$MINTDIR"
		copy_hades_modules "$MINTDIR"
		copy_falcon_modules "$MINTDIR"
		copy_ct60_modules "$MINTDIR"
		# not needed in 060 builds
		rm -rf "$MINTDIR/falcon"
	elif [ "$CPU_TARGET" = "deb" ]
	then
		copy_debug_modules "$MINTDIR" "$CPU_TARGET"
	fi
else
	copy_firebee_modules "$MINTDIR"
fi

copy_xaloader "$XAAESDIR" "$CPU_TARGET"
if [ -n "${VERSIONED+x}" ]
then
	copy_xaaes "$XAAESDIR" "$CPU_TARGET" "yes" "no"
else
	copy_xaaes "$XAAESDIR" "$CPU_TARGET" "no" "no"
fi

copy_usbloader "$USBDIR" "$CPU_TARGET"
copy_usb "$USBDIR" "$CPU_TARGET"
if [ "$CPU_TARGET" != "col" ]
then
	copy_atari_usb_modules "$USBDIR" "$CPU_TARGET"
	copy_blitz_usb_modules "$USBDIR" "$CPU_TARGET"
	if [ "$CPU_TARGET" = "02060" ]
	then
		# EtherNAT
		copy_ct60_usb_modules "$USBDIR" 060
		# CTPCI / Milan / Hades
		copy_ehci_usb_modules "$USBDIR" "$CPU_TARGET"
	elif [ "$CPU_TARGET" = "040" ]
	then
		# Milan / Hades
		copy_ehci_usb_modules "$USBDIR" "$CPU_TARGET"
	elif [ "$CPU_TARGET" = "060" ]
	then
		# EtherNAT
		copy_ct60_usb_modules "$USBDIR" 060
		# CTPCI / Milan / Hades
		copy_ehci_usb_modules "$USBDIR" "$CPU_TARGET"
	elif [ "$CPU_TARGET" = "deb" ]
	then
		copy_ct60_usb_modules "$USBDIR" "$CPU_TARGET"
		copy_ehci_usb_modules "$USBDIR" "$CPU_TARGET"
	fi
else
	copy_ehci_usb_modules "$USBDIR" "$CPU_TARGET"
fi

copy_fonts "$FONTSDIR"
copy_tbl "$TBLDIR"

copy_sysroot "$SYSROOT" "$CPU_TARGET" "$DST/mint"

copy_guides "$GUIDESDIR"

# Atari hardware only
mkdir -p "$SYSROOT/bin"
if test -f "$SRC/sys/xdd/audio/.compile_${CPU_TARGET}/actrl"
then
	cp "$SRC/sys/xdd/audio/.compile_${CPU_TARGET}/actrl" "$SYSROOT/bin/actrl"
fi

if [ -n "${VERSIONED+x}" ]
then
	create_filesystem "$GUIDESDIR"
fi
