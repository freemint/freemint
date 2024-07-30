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
sed -i -e "s/xaloadtargets = 000 02060 030 040 060 col/xaloadtargets = ${TEMP_CPU_TARGET}/;" ./xaaes/src.km/xaloader/XALOADDEFS
sed -i -e "s/xaaestargets = deb 000 sto 030 040 060 col 02060/xaaestargets = ${XAAES_TARGETS}/;" ./xaaes/src.km/XAAESDEFS
sed -i -e "s/moosetargets = 02060 030 040 060 deb 000 col/moosetargets = ${TEMP_CPU_TARGET}/;" ./xaaes/src.km/adi/whlmoose/MOOSEDEFS
sed -i -e "s/kerneltargets = 000 020 030 040 060 deb mil ara hat hat030 col/kerneltargets = ${KERNEL_TARGETS}/;" ./sys/KERNELDEFS
sed -i -e "s/ext2targets = 02060 030 040 060 deb 000 col/ext2targets = ${TEMP_CPU_TARGET}/;" ./sys/xfs/ext2fs/EXT2DEFS
sed -i -e "s/nfstargets = 02060 030 040 060 deb 000 col/nfstargets = ${TEMP_CPU_TARGET}/;" ./sys/xfs/nfs/NFSDEFS
sed -i -e "s/minixtargets = 02060 030 040 060 deb 000 col/minixtargets = ${TEMP_CPU_TARGET}/;" ./sys/xfs/minixfs/MINIXDEFS
sed -i -e "s/xconout2targets = 02060 030 040 060 deb 000 col/xconout2targets = ${TEMP_CPU_TARGET}/;" ./sys/xdd/xconout2/XCONOUT2DEFS
sed -i -e "s/lptargets = 02060 030 040 060 deb 000 col/lptargets = ${TEMP_CPU_TARGET}/;" ./sys/xdd/lp/LPDEFS
sed -i -e "s/usbloadtargets = 000 02060 030 040 060 col/usbloadtargets = ${TEMP_CPU_TARGET}/;" ./sys/usb/src.km/loader/USBLOADDEFS
sed -i -e "s/ethtargets = 02060 030 040 060 deb 000 col prg/ethtargets = ${TEMP_CPU_TARGET}${PRG_TARGET}/;" ./sys/usb/src.km/udd/eth/ETHDEFS
sed -i -e "s/keyboardtargets = 02060 030 040 060 deb 000 col prg/keyboardtargets = ${TEMP_CPU_TARGET}${PRG_TARGET}/;" ./sys/usb/src.km/udd/hid/keyboard/KEYBOARDDEFS
sed -i -e "s/mousetargets = 02060 030 040 060 deb 000 col prg/mousetargets = ${TEMP_CPU_TARGET}${PRG_TARGET}/;" ./sys/usb/src.km/udd/hid/mouse/MOUSEDEFS
sed -i -e "s/printertargets = 02060 030 040 060 deb 000 col prg/printertargets = ${TEMP_CPU_TARGET}${PRG_TARGET}/;" ./sys/usb/src.km/udd/printer/PRINTERDEFS
sed -i -e "s/storagetargets = 02060 030 040 060 deb 000 col prg/storagetargets = ${TEMP_CPU_TARGET}${PRG_TARGET}/;" ./sys/usb/src.km/udd/storage/STORAGEDEFS
sed -i -e "s/usbtargets = 02060 030 040 060 deb 000 col prg plm/usbtargets = ${USB_TARGETS}/;" ./sys/usb/src.km/USBDEFS
sed -i -e "s/ehcitargets = 02060 030 040 060 deb 000 col #prg/ehcitargets = ${TEMP_CPU_TARGET}/;" ./sys/usb/src.km/ucd/ehci/EHCIDEFS
sed -i -e "s/netusbeetargets = 02060 030 040 060 deb 000 prg prg_000 #col/netusbeetargets = ${NETUSBEE_TARGETS}/;" ./sys/usb/src.km/ucd/netusbee/NETUSBEEDEFS
sed -i -e "s/unicorntargets = 02060 030 040 060 deb 000 col prg/unicorntargets = ${TEMP_CPU_TARGET}${PRG_TARGET}/;" ./sys/usb/src.km/ucd/unicorn/UNICORNDEFS
sed -i -e "s/vttusbtargets = 030 deb 000 prg p30 pst mst/vttusbtargets = ${VTTUSB_TARGETS}/;" ./sys/usb/src.km/ucd/vttusb/VTTUSBDEFS
sed -i -e "s/inet4targets = 02060 030 040 060 deb 000 col/inet4targets = ${TEMP_CPU_TARGET}/;" ./sys/sockets/INET4DEFS

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
	tools/sysdir/SYSDIRDEFS \
	tools/usbtool/USBTOOLDEFS \
	; do
	echo "alltargets = ${TEMP_CPU_TARGET}${PRG_TARGET}" >> $f
done
