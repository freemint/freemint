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
copy_modules "$MINTDIR" "02060"
copy_aranym_modules "$MINTDIR"

copy_xaloader "$XAAESDIR" "02060"
if [ -n "${VERSIONED+x}" ]
then
	copy_xaaes "$XAAESDIR" "02060" "yes" "yes"
else
	copy_xaaes "$XAAESDIR" "02060" "no" "yes"
fi

copy_usbloader "$USBDIR" "02060"
copy_usb "$USBDIR" "02060"
copy_aranym_usb_modules "$USBDIR" "040"

copy_fonts "$FONTSDIR"
copy_tbl "$TBLDIR"

copy_sysroot "$SYSROOT" "02060"

copy_guides "$GUIDESDIR"

mkdir -p "$SYSROOT/bin"
cp "$SRC/sys/xdd/audio/.compile_02060/actrl" "$SYSROOT/bin"

############
# filesystem
############

mkdir -p "$DST"

cp -r "$ARANYM_DIR/emutos" "$DST"
cp -r "$ARANYM_DIR/config" "$DST"
cp -r "$ARANYM_DIR"/runme.* "$DST"

sed -e "s/^Bootstrap = mintara.prg/Bootstrap = drive_c\/mint\/$VER\/mintara.prg/;" "$DST/config" > "$DST/config.tmp" && mv "$DST/config.tmp" "$DST/config"

mkdir -p "$ROOT/fvdi"
cp "$FVDI_DIR/auto/fvdi.prg" "$ROOT/fvdi/fvdi.prg"
cp "$FVDI_DIR/fvdiara.sys" "$ROOT/fvdi.sys"
# Match the original FreeMiNT-tuned config: single 1024x768x32@60 mode,
# no interactive resolution menu, no debug/cookie additions. Keep the
# TTF font folder and antialias additions from upstream.
sed -i -f - "$ROOT/fvdi.sys" <<'FVDISED'
# screen width hint: -95 mm -> -90 mm (original)
s|^width -95$|width -90|
# screen height hint: -95 mm -> -90 mm (original)
s|^height -95$|height -90|
# re-enable single Trap #2 bend (was active in original)
s|^#singlebend$|singlebend|
# drop "q/b/d" startup help lines
/^echo "[qbd] - /s|^|#|
# drop default-key seeds (n for menus, 1 for resolution)
/^setkey [n1]$/s|^|#|
# drop "press q to skip fVDI" feature
/^exitkey q$/s|^|#|
# drop fake NVDI/Speedo cookies
/^cookie /s|^|#|
# drop vqgdos _FSM probe
/^vqgdos /s|^|#|
# drop per-opcode debug-output suppressions
/^silent /s|^|#|
# drop interactive resolution menu echo lines
/^echo "[0-9] - /s|^|#|
# drop swapkey pairs that drive the resolution menu
/^swapkey$/s|^|#|
# drop casekey 0..9 jumps into the resolution menu
/^casekey [0-9] res/s|^|#|
# pin the active driver line to the original mode/flags
s|^01r aranym.sys mode 1024x768x32@72 irq accelerate xxxxxxx$|01r aranym.sys mode 1024x768x32@60|
# comment out the other 01r lines
/^01r aranym\.sys mode .* irq accelerate xxxxxxx$/s|^|#|
/^goto font_specification$/s|^|#|
FVDISED
mkdir -p "$ROOT/gemsys"
cp "$FVDI_DIR/gemsys/aranym.sys" "$ROOT/gemsys/aranym.sys"
sed -e "/^#exec u:\/opt\/GEM\/gluestik\/gluestik.prg/a#echo\n\nexec u:\/c\/fvdi\/fvdi.prg" "$MINTDIR/mint.cnf" > "$MINTDIR/mint.cnf.tmp" && mv "$MINTDIR/mint.cnf.tmp" "$MINTDIR/mint.cnf"

mkdir -p "$SYSROOT/bin"
cp "$ARANYM_DIR/nfeth-config/eth0-config.sh" "$SYSROOT/bin"
cp "$ARANYM_DIR/nfeth-config/nfeth-config" "$SYSROOT/bin"
sed -e "s/\.\/nfeth-config/nfeth-config/g;" "$SYSROOT/bin/eth0-config.sh" > "$SYSROOT/bin/eth0-config.sh.tmp" && mv "$SYSROOT/bin/eth0-config.sh.tmp" "$SYSROOT/bin/eth0-config.sh"
sed -e "s/#exec u:\/bin\/bash u:\/bin\/eth0-config.sh/exec u:\/bin\/bash u:\/bin\/eth0-config.sh/;" "$MINTDIR/mint.cnf" > "$MINTDIR/mint.cnf.tmp" && mv "$MINTDIR/mint.cnf.tmp" "$MINTDIR/mint.cnf"

create_filesystem "$GUIDESDIR"
