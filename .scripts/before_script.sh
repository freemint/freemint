#!/bin/bash -ex
# -e: Exit immediately if a command exits with a non-zero status.
# -x: Display expanded script commands

if [ -n "${VERSIONED+x}" ]
then
	sed -i -e "s/\"cur\"/\"${SHORT_ID}\"/;" sys/buildinfo/version.h
fi
gcc -I"sys" -o /tmp/version $SCRIPT_DIR/version.c

. "$SCRIPT_DIR/kernel_targets.sh"
. "$SCRIPT_DIR/xaaes_targets.sh"
. "$SCRIPT_DIR/usb_targets.sh"
. "$SCRIPT_DIR/netusbee_targets.sh"
. "$SCRIPT_DIR/vttusb_targets.sh"

TEMP_CPU_TARGET="$CPU_TARGET"
if [ "$CPU_TARGET" = "prg" ]
then
	PRG_TARGET="$CPU_TARGET"
	TEMP_CPU_TARGET=""
elif [ "$CPU_TARGET" = "ara" ]
then
	TEMP_CPU_TARGET="040"
fi

# limit building to selected CPU targets
echo "xaloadtargets = ${TEMP_CPU_TARGET}" >> ./xaaes/src.km/xaloader/XALOADDEFS
echo "xaaestargets = ${XAAES_TARGETS}" >> ./xaaes/src.km/XAAESDEFS
echo "moosetargets = ${TEMP_CPU_TARGET}" >> ./xaaes/src.km/adi/whlmoose/MOOSEDEFS
echo "kerneltargets = ${KERNEL_TARGETS}" >> ./sys/KERNELDEFS
echo "ext2targets = ${TEMP_CPU_TARGET}" >> ./sys/xfs/ext2fs/EXT2DEFS
echo "nfstargets = ${TEMP_CPU_TARGET}" >> ./sys/xfs/nfs/NFSDEFS
echo "minixtargets = ${TEMP_CPU_TARGET}" >> ./sys/xfs/minixfs/MINIXDEFS
echo "xconout2targets = ${TEMP_CPU_TARGET}" >> ./sys/xdd/xconout2/XCONOUT2DEFS
echo "lptargets = ${TEMP_CPU_TARGET}" >> ./sys/xdd/lp/LPDEFS
echo "usbloadtargets = ${TEMP_CPU_TARGET}" >> ./sys/usb/src.km/loader/USBLOADDEFS
echo "ethtargets = ${TEMP_CPU_TARGET}${PRG_TARGET}" >> ./sys/usb/src.km/udd/eth/ETHDEFS
echo "keyboardtargets = ${TEMP_CPU_TARGET}${PRG_TARGET}" >> ./sys/usb/src.km/udd/hid/keyboard/KEYBOARDDEFS
echo "mousetargets = ${TEMP_CPU_TARGET}${PRG_TARGET}" >> ./sys/usb/src.km/udd/hid/mouse/MOUSEDEFS
echo "printertargets = ${TEMP_CPU_TARGET}${PRG_TARGET}" >> ./sys/usb/src.km/udd/printer/PRINTERDEFS
echo "storagetargets = ${TEMP_CPU_TARGET}${PRG_TARGET}" >> ./sys/usb/src.km/udd/storage/STORAGEDEFS
echo "usbtargets = ${USB_TARGETS}" >> ./sys/usb/src.km/USBDEFS
echo "ehcitargets = ${TEMP_CPU_TARGET}" >> ./sys/usb/src.km/ucd/ehci/EHCIDEFS
echo "netusbeetargets = ${NETUSBEE_TARGETS}" >> ./sys/usb/src.km/ucd/netusbee/NETUSBEEDEFS
echo "unicorntargets = ${TEMP_CPU_TARGET}${PRG_TARGET}" >> ./sys/usb/src.km/ucd/unicorn/UNICORNDEFS
echo "vttusbtargets = ${VTTUSB_TARGETS}" >> ./sys/usb/src.km/ucd/vttusb/VTTUSBDEFS
echo "inet4targets = ${TEMP_CPU_TARGET}" >> ./sys/sockets/INET4DEFS

for f in tools/IO/IODEFS \
	tools/crypto/CRYPTODEFS \
	tools/fdisk/FDISKDEFS \
	tools/fsetter/FSETTERDEFS \
	tools/gluestik/GLUESTIKDEFS \
	tools/lpflush/LPFLUSHDEFS \
	tools/mgw/MGWDEFS \
	tools/minix/MINIXDEFS \
	tools/mintload/MINTLOADDEFS \
	tools/mkfatfs/MKFATFSDEFS \
	tools/mktbl/MKTBLDEFS \
	tools/net-tools/NETTOOLSDEFS \
	tools/nfs/NFSDEFS \
	tools/nohog2/NOHOG2DEFS \
	tools/strace/STRACEDEFS \
	tools/swkbdtbl/SWKBDTBLDEFS \
	tools/sysctl/SYSCTLDEFS \
	tools/usbtool/USBTOOLDEFS \
	tools/ps/PSDEFS \
	; do
	echo "alltargets = ${TEMP_CPU_TARGET}${PRG_TARGET}" >> $f
done
