#!/bin/bash -eux
# -e: Exit immediately if a command exits with a non-zero status.
# -u: Treat unset variables as an error when substituting.
# -x: Display expanded script commands

SRC="$1"
DST="$2"
VER="$3"
ARANYM_DIR="$4"

. "$SCRIPT_DIR/util.sh"

ROOT="$DST/drive_c"
GUIDESDIR="$ROOT/guides"
MINTDIR="$ROOT/mint/$VER"
XAAESDIR="$MINTDIR/xaaes"
SYSROOT="$MINTDIR/sys-root"
USBDIR="$MINTDIR/usb"
TBLDIR="$MINTDIR/keyboard"
FONTSDIR="$MINTDIR/fonts"

copy_kernel "$MINTDIR" "ara"
if [ -n "${VERSIONED+x}" ]
then
	copy_mint_cnf "$MINTDIR"
fi
copy_modules "$MINTDIR" "040"
copy_aranym_modules "$MINTDIR"

copy_xaloader "$XAAESDIR" "040"
if [ -n "${VERSIONED+x}" ]
then
	copy_xaaes "$XAAESDIR" "040" "yes" "yes"
else
	copy_xaaes "$XAAESDIR" "040" "no" "yes"
fi

copy_usbloader "$USBDIR" "040"
copy_usb "$USBDIR" "040"
copy_aranym_usb_modules "$USBDIR"

copy_fonts "$FONTSDIR"
copy_tbl "$TBLDIR"

copy_sysroot "$SYSROOT" "040" "$ROOT/mint"

copy_guides "$GUIDESDIR"

mkdir -p "$SYSROOT/bin"
cp "$SRC/sys/xdd/audio/actrl" "$SYSROOT/bin"

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
sed -e "/^#exec u:\/opt\/GEM\/gluestik\/gluestik.prg/a#echo\n\nexec u:\/c\/fvdi\/fvdi.prg" "$MINTDIR/mint.cnf" > "$MINTDIR/mint.cnf.tmp" && mv "$MINTDIR/mint.cnf.tmp" "$MINTDIR/mint.cnf"

mkdir -p "$SYSROOT/bin"
cp "$ARANYM_DIR/nfeth-config/eth0-config.sh" "$SYSROOT/bin"
cp "$ARANYM_DIR/nfeth-config/nfeth-config" "$SYSROOT/bin"
sed -e "s/\.\/nfeth-config/nfeth-config/g;" "$SYSROOT/bin/eth0-config.sh" > "$SYSROOT/bin/eth0-config.sh.tmp" && mv "$SYSROOT/bin/eth0-config.sh.tmp" "$SYSROOT/bin/eth0-config.sh"
sed -e "s/#exec u:\/bin\/bash u:\/bin\/eth0-config.sh/exec u:\/bin\/bash u:\/bin\/eth0-config.sh/;" "$MINTDIR/mint.cnf" > "$MINTDIR/mint.cnf.tmp" && mv "$MINTDIR/mint.cnf.tmp" "$MINTDIR/mint.cnf"

create_filesystem "$GUIDESDIR"
