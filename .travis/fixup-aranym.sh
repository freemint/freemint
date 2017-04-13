#!/bin/sh

SRC="$1"
DST="$2"
CUR="$3"
VER="$4"
ARANYM_DIR="$5"

PWD="$(dirname "$0")"
. "$PWD/util.sh"

ROOT="$DST/drive_c"
MINTDIR="$ROOT/mint/$VER"
XAAESDIR="$MINTDIR/xaaes"
SYSROOT="$MINTDIR/sysroot"
USBDIR="$MINTDIR/usb"

copy_xaloader "$XAAESDIR"

copy_usbloader "$USBDIR"

copy_sysroot "$SYSROOT"
