#!/bin/sh

SRC="$1"
DST="$2"
CUR="$3"
VER="$4"
ARANYM_DIR="$5"

PWD="$(dirname "$0")"
. "$PWD/util.sh"

ROOT="$DST/drive_c"
MINTDIR_020="$ROOT/mint/$VER"
XAAESDIR_020="$MINTDIR_020/xaaes"
SYSROOT_020="$MINTDIR_020/sysroot"
USBDIR_020="$MINTDIR_020/usb"
TBLDIR_020="$MINTDIR_020/tbl"
FONTSDIR_020="$MINTDIR_020/fonts"

copy_kernel "$MINTDIR_020" "ara"
copy_modules "$MINTDIR_020" "02060"
copy_aranym_modules "$MINTDIR_020"

# TODO: copy_xaloader "$XAAESDIR_020" (done as a separate step)
copy_xaaes "$XAAESDIR_020" "02060"

# TODO: copy_usbloader "$USBDIR_020" (done as a separate step)
copy_usb "$USBDIR_020" "02060"
# unfortunately the usb loader isn't aware of SYSDIR or MCHDIR
cp "$SRC/sys/usb/src.km/ucd/aranym/aranym.ucd" "$USBDIR_020"

copy_fonts "$FONTSDIR_020"
copy_tbl "$TBLDIR_020"

# TODO: copy_sysroot "$SYSROOT_020" (done as a separate step)
mkdir -p "$SYSROOT_020/bin"
cp "$SRC/sys/xdd/audio/actrl" "$SYSROOT_020/bin/actrl.ttp"

############
# filesystem
############

mkdir -p "$DST"

cp -r "$ARANYM_DIR/emutos" "$DST"
cp -r "$ARANYM_DIR/config" "$DST"
cp -r "$ARANYM_DIR"/runme.* "$DST"

sed -e "s/^Bootstrap = mintara.prg/Bootstrap = drive_c\/mint\/$VER\/mintara.prg/;" "$DST/config" > "$DST/config.tmp" && mv "$DST/config.tmp" "$DST/config"

mkdir -p "$ROOT"
cp -r "$ARANYM_DIR/fvdi"/* "$ROOT"
sed -e "/^#exec u:\/c\/mint\/gluestik.prg/a#echo\n\nexec u:\/c\/fvdi\/fvdi.prg" "$MINTDIR_020/mint.cnf" > "$MINTDIR_020/mint.cnf.tmp" && mv "$MINTDIR_020/mint.cnf.tmp" "$MINTDIR_020/mint.cnf"

cp -r "$ARANYM_DIR/teradesk" "$ROOT"
sed -e "s/^#setenv AVSERVER   \"DESKTOP \"/setenv AVSERVER   \"DESKTOP \"/;" "$XAAESDIR_020/xaaes.cnf" > "$XAAESDIR_020/xaaes.cnf.tmp" && mv "$XAAESDIR_020/xaaes.cnf.tmp" "$XAAESDIR_020/xaaes.cnf"
sed -e "s/^#setenv FONTSELECT \"DESKTOP \"/setenv FONTSELECT \"DESKTOP \"/;" "$XAAESDIR_020/xaaes.cnf" > "$XAAESDIR_020/xaaes.cnf.tmp" && mv "$XAAESDIR_020/xaaes.cnf.tmp" "$XAAESDIR_020/xaaes.cnf"
sed -e "s/^#shell = c:\\\\teradesk\\\\desktop.prg/shell = c:\\\\teradesk\\\\desktop.prg/;" "$XAAESDIR_020/xaaes.cnf" > "$XAAESDIR_020/xaaes.cnf.tmp" && mv "$XAAESDIR_020/xaaes.cnf.tmp" "$XAAESDIR_020/xaaes.cnf"
