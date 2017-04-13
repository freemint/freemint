#!/bin/sh

# helper script to copy files outside the .compile_${CPU} scheme

CPU="$1"
SRC="$2"
DST="$3"
CUR="$4"
VER="$5"

PWD="$(dirname "$0")"
. "$PWD/util.sh"

AUTODIR="$DST/$CPU/auto"
MINTDIR="$DST/$CPU/mint/$VER"
XAAESDIR="$MINTDIR/xaaes"
SYSROOT="$MINTDIR/sysroot"
USBDIR="$MINTDIR/usb"

copy_auto "$AUTODIR"

copy_xaloader "$XAAESDIR"

copy_usbloader "$USBDIR"

copy_sysroot "$SYSROOT"
