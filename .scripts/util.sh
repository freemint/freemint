# use as: . util.sh

copy_auto() {
	local AUTODIR="$1"
	local TARGET="$2"
	local CUR="$3"
	mkdir -p "$AUTODIR"
	cp "$SRC/tools/mintload/.compile_$TARGET/mintload.prg" "$AUTODIR/mint$CUR.prg"
}

copy_kernel() {
	local MINTDIR="$1"
	mkdir -p "$MINTDIR"
	shift
	for TARGET in $*
	do
		cp "$SRC/sys/.compile_$TARGET"/mint*.prg "$MINTDIR"
	done
}

copy_mint_cnf() {
	local MINTDIR="$1"
	mkdir -p "$MINTDIR"
	cp "$SRC/doc/examples/mint.cnf" "$MINTDIR"
	sed -e "s/#setenv LOGNAME root/setenv LOGNAME root/;" "$MINTDIR/mint.cnf" > "$MINTDIR/mint.cnf.tmp" && mv "$MINTDIR/mint.cnf.tmp" "$MINTDIR/mint.cnf"
	sed -e "s/#setenv USER    root/setenv USER    root/;" "$MINTDIR/mint.cnf" > "$MINTDIR/mint.cnf.tmp" && mv "$MINTDIR/mint.cnf.tmp" "$MINTDIR/mint.cnf"
	sed -e "s/#setenv HOME    \/root/setenv HOME    \/root/;" "$MINTDIR/mint.cnf" > "$MINTDIR/mint.cnf.tmp" && mv "$MINTDIR/mint.cnf.tmp" "$MINTDIR/mint.cnf"
	sed -e "s/#setenv SHELL   \/bin\/bash/setenv SHELL   \/bin\/bash/;" "$MINTDIR/mint.cnf" > "$MINTDIR/mint.cnf.tmp" && mv "$MINTDIR/mint.cnf.tmp" "$MINTDIR/mint.cnf"
}

copy_modules() {
	local MINTDIR="$1"
	local TARGET="$2"
	mkdir -p "$MINTDIR"
	cp "$SRC/sys/sockets/.compile_$TARGET/inet4.xdd" "$MINTDIR"
	cp "$SRC/sys/xdd/lp/.compile_$TARGET/lp.xdd" "$MINTDIR"
	cp "$SRC/sys/xdd/xconout2/.compile_$TARGET/xconout2.xdd" "$MINTDIR"
	cp "$SRC/sys/xfs/ext2fs/.compile_$TARGET/ext2.xfs" "$MINTDIR"
	cp "$SRC/sys/xfs/minixfs/.compile_$TARGET/minix.xfs" "$MINTDIR/minix.xfx"
	cp "$SRC/sys/xfs/nfs/.compile_$TARGET/nfs.xfs" "$MINTDIR"
	cp "$SRC/sys/xfs/isofs/.compile_$TARGET/isofs.xfs" "$MINTDIR/isofs.xfx"
}

# modules compatible with all m68k machines (except the FireBee...)
copy_m68k_modules() {
	local SYSDIR="$1"
	local TARGET="$2"
	mkdir -p "$SYSDIR"
	cp "$SRC/sys/sockets/xif/asix_${TARGET}.xif" "$SYSDIR/asix.xix"
	cp "$SRC/sys/sockets/xif/picowifi_${TARGET}.xif" "$SYSDIR/picowifi.xix"
	cp "$SRC/sys/sockets/xif/plip_${TARGET}.xif" "$SYSDIR/plip.xif"
	cp "$SRC/sys/sockets/xif/slip_${TARGET}.xif" "$SYSDIR/slip.xif"
	cp "$SRC/sys/xdd/audio/.compile_$TARGET/audiodev.xdd" "$SYSDIR"
	cp "$SRC/sys/xdd/flop-raw/.compile_$TARGET/flop_raw.xdd" "$SYSDIR"
}

copy_st_modules() {
	local MCHDIR="$1/st"
	mkdir -p "$MCHDIR"
	# TODO: perhaps these four are compatible also with other machines
	#       but they are awfully old and obsolete so let's keep them here
	cp "$SRC/sys/sockets/xif/biodma_000.xif" "$MCHDIR/biodma.xix"
	cp "$SRC/sys/sockets/xif/de600_000.xif" "$MCHDIR/de600.xix"
	cp "$SRC/sys/sockets/xif/dial_000.xif" "$MCHDIR/dial.xix"
	cp "$SRC/sys/sockets/xif/pamsdma_000.xif" "$MCHDIR/pamsdma.xix"
	cp "$SRC/sys/sockets/xif/rtl8012st_000.xif" "$MCHDIR/rtl8012st.xix"
	cp "$SRC/sys/xdd/mfp/.compile_000/mfp.xdd" "$MCHDIR"
}
copy_megast_modules() {
	local MCHDIR="$1/megast"
	mkdir -p "$MCHDIR"
	cp "$SRC/sys/sockets/xif/lance_000.xif" "$MCHDIR/lance.xix"
	cp "$SRC/sys/sockets/xif/rieblmst_000.xif" "$MCHDIR/riebl.xix"
	cp "$SRC/sys/sockets/xif/rieblmst_fast_000.xif" "$MCHDIR/riebl_fast.xix"
	cp "$SRC/sys/sockets/xif/rieblspc_000.xif" "$MCHDIR/rieblspc.xix"
	cp "$SRC/sys/sockets/xif/rieblspc_fast_000.xif" "$MCHDIR/riebls_fast.xix"
	cp "$SRC/sys/sockets/xif/rtl8012st_000.xif" "$MCHDIR/rtl8012st.xix"
	cp "$SRC/sys/xdd/mfp/.compile_000/mfp.xdd" "$MCHDIR"
}
copy_ste_modules() {
	local MCHDIR="$1/ste"
	mkdir -p "$MCHDIR"
	cp "$SRC/sys/sockets/xif/rtl8012st_000.xif" "$MCHDIR/rtl8012st.xix"
	cp "$SRC/sys/xdd/mfp/.compile_000/mfp.xdd" "$MCHDIR"
}
copy_megaste_modules() {
	local MCHDIR="$1/megaste"
	mkdir -p "$MCHDIR"
	cp "$SRC/sys/sockets/xif/rieblste_000.xif" "$MCHDIR/rieblste.xix"
	cp "$SRC/sys/sockets/xif/rtl8012st_000.xif" "$MCHDIR/rtl8012st.xix"
	cp "$SRC/sys/xdd/mfp/.compile_000/mfp.xdd" "$MCHDIR"
	cp "$SRC/sys/xdd/scc/.compile_000/scc.xdd" "$MCHDIR"
}
copy_tt_modules() {
	local MCHDIR="$1/tt"
	mkdir -p "$MCHDIR"
	cp "$SRC/sys/sockets/xif/daynaport/.compile_030/scsilink.xif" "$MCHDIR/scsilink.xix"
	cp "$SRC/sys/sockets/xif/lance_030.xif" "$MCHDIR/lance.xix"
	cp "$SRC/sys/sockets/xif/riebltt_030.xif" "$MCHDIR/riebltt.xix"
	cp "$SRC/sys/sockets/xif/rtl8012_030.xif" "$MCHDIR/rtl8012.xix"
	cp "$SRC/sys/xdd/mfp/.compile_030/mfp.xdd" "$MCHDIR"
	cp "$SRC/sys/xdd/scc/.compile_030/scc.xdd" "$MCHDIR"
}
copy_falcon_modules() {
	local MCHDIR="$1/falcon"
	mkdir -p "$MCHDIR"
	cp "$SRC/sys/sockets/xif/rtl8012_02060.xif" "$MCHDIR/rtl8012.xix"
	cp "$SRC/sys/sockets/xif/daynaport/.compile_02060/scsilink.xif" "$MCHDIR/scsilink.xix"
	cp "$SRC/sys/xdd/dsp56k/.compile_02060/dsp56k.xdd" "$MCHDIR"
	cp "$SRC/sys/xdd/scc/.compile_02060/scc.xdd" "$MCHDIR"
}
copy_milan_modules() {
	local MCHDIR="$1/milan"
	mkdir -p "$MCHDIR"
	cp "$SRC/sys/xdd/uart/.compile_mil/uart.xdd" "$MCHDIR"
	cp "$SRC/sys/xdd/mfp/.compile_mil/mfp.xdd" "$MCHDIR"
}
copy_hades_modules() {
	local MCHDIR="$1/hades"
	mkdir -p "$MCHDIR"
	# Hades ESCC is compatible with TT SCC
	cp "$SRC/sys/xdd/scc/.compile_02060/scc.xdd" "$MCHDIR"
}
copy_ct60_modules() {
	local MCHDIR="$1/ct60"
	mkdir -p "$MCHDIR"

	cp -r "$1/falcon"/* "$MCHDIR"

	# we don't need plain st/falcon version
	rm "$MCHDIR/rtl8012.xix"
	cp "$SRC/sys/sockets/xif/rtl8012_060.xif" "$MCHDIR/rtl8012.xix"
	cp "$SRC/sys/sockets/xif/rtl8139_060.xif" "$MCHDIR/rtl8139.xix"
	cp "$SRC/sys/sockets/xif/ethernat/.compile_060/ethernat.xif" "$MCHDIR/ethernat.xix"
	cp "$SRC/sys/sockets/xif/svethlana/.compile_060/svethlan.xif" "$MCHDIR/svethlan.xix"
}
copy_firebee_modules() {
	local MCHDIR="$1/firebee"
	mkdir -p "$MCHDIR"
	cp "$SRC/sys/sockets/xif/fec/.compile_v4e/fec.xif" "$MCHDIR/fec.xif"
}
copy_aranym_modules() {
	local MCHDIR="$1/aranym"
	mkdir -p "$MCHDIR"
	cp "$SRC/sys/sockets/xif/nfeth/.compile_040/nfeth.xif" "$MCHDIR"
	cp "$SRC/sys/xdd/nfstderr/.compile_040/nfstderr.xdd" "$MCHDIR"
	cp "$SRC/sys/xdd/nfexec/.compile_040/nfexec.xdd" "$MCHDIR"
	cp "$SRC/sys/xdd/nfcdrom/.compile_040/nfcdrom.xdd" "$MCHDIR/nfcdrom.xdx"
	cp "$SRC/sys/xfs/aranym/aranym.xfs" "$MCHDIR"
	# not really needed
	cp "$SRC/sys/xfs/hostfs/.compile_000/hostfs.xfs" "$MCHDIR/hostfs.xfx"
}
copy_debug_modules() {
	local SYSDIR="$1"
	local TARGET="$2"
	mkdir -p "$SYSDIR"
	cp "$SRC/sys/xdd/scc/.compile_${TARGET}/scc.xdd" "$SYSDIR"
}

copy_xaloader() {
	local XAAESDIR="$1"
	local TARGET="$2"
	mkdir -p "$XAAESDIR"
	cp "$SRC/xaaes/src.km/xaloader/.compile_$TARGET/xaloader.prg" "$XAAESDIR"
}

copy_xaaes() {
	local XAAESDIR="$1"
	local TARGET="$2"
	local BOOTABLE="$3"
	local ARANYM="$4"
	mkdir -p "$XAAESDIR"
	if [ "$TARGET" = "col" ]
	then
	cp "$SRC/xaaes/src.km/.compile_$TARGET/xaaesv4e.km" "$XAAESDIR/xaaes.km"
	elif [ "$TARGET" = "000" ]
	then
	cp "$SRC/xaaes/src.km/.compile_sto/xaaesst.km" "$XAAESDIR/xaaesst.km"
	cp "$SRC/xaaes/src.km/.compile_$TARGET/xaaes$TARGET.km" "$XAAESDIR/xaaes.km"
	else
	cp "$SRC/xaaes/src.km/.compile_$TARGET/xaaes$TARGET.km" "$XAAESDIR/xaaes.km"
	fi
	cp "$SRC/xaaes/src.km/adi/whlmoose/.compile_$TARGET/moose.adi" "$XAAESDIR"
	cp "$SRC/xaaes/src.km/adi/whlmoose/.compile_$TARGET/moose_w.adi" "$XAAESDIR"
	mkdir -p "$XAAESDIR/gradient"
	cp "$SRC/xaaes/src.km/gradient"/*.grd "$XAAESDIR/gradient"
	cp -r "$SRC/xaaes/src.km/img" "$XAAESDIR"
	cp -r "$SRC/xaaes/src.km/pal" "$XAAESDIR"
	mkdir -p "$XAAESDIR/widgets"
	cp "$SRC/xaaes/src.km/widgets"/*.rsc "$XAAESDIR/widgets"
	mkdir -p "$XAAESDIR/xobj"
	cp "$SRC/xaaes/src.km/xobj"/*.rsc "$XAAESDIR/xobj"
	if [ "$BOOTABLE" = "yes" ]
	then
	cp "$SRC/xaaes/src.km/example.cnf" "$XAAESDIR/xaaes.cnf"
	sed -e "s/^#setenv AVSERVER   \"DESKTOP \"/setenv AVSERVER   \"DESKTOP \"/;" "$XAAESDIR/xaaes.cnf" > "$XAAESDIR/xaaes.cnf.tmp" && mv "$XAAESDIR/xaaes.cnf.tmp" "$XAAESDIR/xaaes.cnf"
	sed -e "s/^#setenv FONTSELECT \"DESKTOP \"/setenv FONTSELECT \"DESKTOP \"/;" "$XAAESDIR/xaaes.cnf" > "$XAAESDIR/xaaes.cnf.tmp" && mv "$XAAESDIR/xaaes.cnf.tmp" "$XAAESDIR/xaaes.cnf"
	if [ "$TARGET" = "col" ]
	then
	sed -e "s/^#shell = c:\\\\teradesk\\\\desktop.prg/shell = u:\\\\opt\\\\GEM\\\\teradesk\\\\desk_cf.prg/;" "$XAAESDIR/xaaes.cnf" > "$XAAESDIR/xaaes.cnf.tmp" && mv "$XAAESDIR/xaaes.cnf.tmp" "$XAAESDIR/xaaes.cnf"
	else
	sed -e "s/^#shell = c:\\\\teradesk\\\\desktop.prg/shell = u:\\\\opt\\\\GEM\\\\teradesk\\\\desktop.prg/;" "$XAAESDIR/xaaes.cnf" > "$XAAESDIR/xaaes.cnf.tmp" && mv "$XAAESDIR/xaaes.cnf.tmp" "$XAAESDIR/xaaes.cnf"
	fi
	if [ "$ARANYM" = "yes" ]
	then
	sed -e "s/^clipboard = c:\\\\clipbrd\\\\/clipboard = u:\\\\host\\\\clipbrd\\\\/;" "$XAAESDIR/xaaes.cnf" > "$XAAESDIR/xaaes.cnf.tmp" && mv "$XAAESDIR/xaaes.cnf.tmp" "$XAAESDIR/xaaes.cnf"
	fi
	fi
	cp "$SRC/xaaes/src.km"/xa_help.* "$XAAESDIR"
	cp "$SRC/xaaes/src.km"/*.rsc "$XAAESDIR"
	cp "$SRC/xaaes/src.km"/*.rsl "$XAAESDIR"
}

copy_usbloader() {
	local USBDIR="$1"
	local TARGET="$2"
	mkdir -p "$USBDIR"
	cp "$SRC/sys/usb/src.km/loader/.compile_$TARGET/loader.prg" "$USBDIR"
}

copy_usb() {
	local USBDIR="$1"
	local TARGET="$2"
	mkdir -p "$USBDIR"
	cp "$SRC/sys/usb/src.km/.compile_$TARGET"/*.km "$USBDIR/usb.km"
	cp "$SRC/sys/usb/src.km/udd/eth/.compile_$TARGET/eth.udd" "$USBDIR"
	cp "$SRC/sys/usb/src.km/udd/hid/keyboard/.compile_$TARGET/keyboard.udd" "$USBDIR"
	cp "$SRC/sys/usb/src.km/udd/hid/mouse/.compile_$TARGET/mouse.udd" "$USBDIR"
	cp "$SRC/sys/usb/src.km/udd/hid/tablet/.compile_$TARGET/tablet.udd" "$USBDIR"
	cp "$SRC/sys/usb/src.km/udd/printer/.compile_$TARGET/printer.udd" "$USBDIR"
	cp "$SRC/sys/usb/src.km/udd/storage/.compile_$TARGET/storage.udd" "$USBDIR"
}

copy_atari_usb_modules() {
	local USBDIR="$1"
	local TARGET="$2"
	mkdir -p "$USBDIR"
	cp "$SRC/sys/usb/src.km/ucd/netusbee/.compile_$TARGET/netusbee.ucd" "$USBDIR"
	cp "$SRC/sys/usb/src.km/ucd/unicorn/.compile_$TARGET/unicorn.ucd" "$USBDIR"
}

copy_blitz_usb_modules() {
	local USBDIR="$1"
	local TARGET="$2"
	mkdir -p "$USBDIR"
	if [ "$TARGET" = "000" ]
	then
	cp "$SRC/sys/usb/src.km/ucd/vttusb/.compile_$TARGET/blitz.ucd" "$USBDIR"
	cp "$SRC/sys/usb/src.km/ucd/vttusb/.compile_mst/blitz_st.ucd" "$USBDIR"
	elif [ "$TARGET" = "02060" ] || [ "$TARGET" = "030" ]
	then
	cp "$SRC/sys/usb/src.km/ucd/vttusb/.compile_030/blitz030.ucd" "$USBDIR"
	elif [ "$TARGET" = "deb" ]
	then
	cp "$SRC/sys/usb/src.km/ucd/vttusb/.compile_$TARGET/blitz"*.ucd "$USBDIR"
	fi
}

copy_ehci_usb_modules() {
	local USBDIR="$1"
	local TARGET="$2"
	mkdir -p "$USBDIR"
	cp "$SRC/sys/usb/src.km/ucd/ehci/.compile_$TARGET/ehci.ucd" "$USBDIR"
}

copy_ct60_usb_modules() {
	local USBDIR="$1"
	local TARGET="$2"
	mkdir -p "$USBDIR"
	cp "$SRC/sys/usb/src.km/ucd/ethernat/.compile_${TARGET}/ethernat.ucd" "$USBDIR"
}

copy_aranym_usb_modules() {
	local USBDIR="$1"
	local TARGET="$2"
	mkdir -p "$USBDIR"
	cp "$SRC/sys/usb/src.km/ucd/aranym/.compile_${TARGET}/aranym.ucd" "$USBDIR"
}

copy_usb4tos() {
	local USB4TOSDIR="$1"
	mkdir -p "$USB4TOSDIR"
	cp "$SRC/sys/usb/src.km/.compile_prg/usb.prg" "$USB4TOSDIR"
	cp "$SRC/sys/usb/src.km/.compile_plm/usb_lmem.prg" "$USB4TOSDIR"
	cp "$SRC/sys/usb/src.km/ucd/aranym/.compile_prg/aranym.prg" "$USB4TOSDIR"
	cp "$SRC/sys/usb/src.km/ucd/unicorn/.compile_prg/unicorn.prg" "$USB4TOSDIR"
	cp "$SRC/sys/usb/src.km/ucd/netusbee/.compile_prg/netusbee.prg" "$USB4TOSDIR"
	cp "$SRC/sys/usb/src.km/ucd/netusbee/.compile_prg_000/netusbee.prg" "$USB4TOSDIR/netus000.prg"
	cp "$SRC/sys/usb/src.km/ucd/ethernat/.compile_prg/ethernat.prg" "$USB4TOSDIR"
	cp "$SRC/sys/usb/src.km/ucd/vttusb/.compile_prg/blitz.prg" "$USB4TOSDIR"
	cp "$SRC/sys/usb/src.km/ucd/vttusb/.compile_p30/blitz030.prg" "$USB4TOSDIR"
	cp "$SRC/sys/usb/src.km/ucd/vttusb/.compile_pst/blitz_st.prg" "$USB4TOSDIR"
	#cp "$SRC/sys/usb/src.km/udd/eth/.compile_prg/eth.prg" "$USB4TOSDIR"
	cp "$SRC/sys/usb/src.km/udd/hid/keyboard/.compile_prg/keyboard.prg" "$USB4TOSDIR"
	cp "$SRC/sys/usb/src.km/udd/hid/mouse/.compile_prg/mouse.prg" "$USB4TOSDIR"
	cp "$SRC/sys/usb/src.km/udd/hid/tablet/.compile_prg/tablet.prg" "$USB4TOSDIR"
	cp "$SRC/sys/usb/src.km/udd/printer/.compile_prg/printer.prg" "$USB4TOSDIR"
	cp "$SRC/sys/usb/src.km/udd/storage/.compile_prg/storage.prg" "$USB4TOSDIR"
	# TODO: multiple CPU variants?
	cp "$SRC/tools/usbtool/.compile_prg/usbtool.acc" "$USB4TOSDIR"
}

copy_fonts() {
	local FONTSDIR="$1"
	mkdir -p "$FONTSDIR"
	cp -r "$SRC/fonts"/* "$FONTSDIR"
	# patch long file names
	mv "$FONTSDIR/cs/cp1250_08.txt" "$FONTSDIR/cs/cp125008.txt"
	mv "$FONTSDIR/cs/cp1250_09.txt" "$FONTSDIR/cs/cp125009.txt"
	mv "$FONTSDIR/cs/cp1250_10.txt" "$FONTSDIR/cs/cp125010.txt"
	mv "$FONTSDIR/pl/ISO-8859-2.fnt" "$FONTSDIR/pl/iso88592.fnt"
}

copy_tbl() {
	local TBLDIR="$1"
	mkdir -p "$TBLDIR"
	cp -r "$SRC/sys/tbl"/* "$TBLDIR"
}

copy_sysroot() {
	local SYSROOT="$1"
	local TARGET="$2"

	mkdir -p "$SYSROOT/opt/GEM"

	mkdir -p "$SYSROOT/opt/GEM/fsetter"
	cp "$SRC/tools/fsetter/.compile_$TARGET/fsetter.app" "$SYSROOT/opt/GEM/fsetter"
	cp "$SRC/tools/fsetter/liesmich" "$SYSROOT/opt/GEM/fsetter"
	cp "$SRC/tools/fsetter/readme" "$SYSROOT/opt/GEM/fsetter"
	cp "$SRC/tools/fsetter/history.txt" "$SYSROOT/opt/GEM/fsetter"
	cp "$SRC/tools/fsetter/fsetter_e.rsc" "$SYSROOT/opt/GEM/fsetter/fsetter.rsc"
	mkdir -p "$SYSROOT/opt/GEM/fsetter/de"
	cp "$SRC/tools/fsetter/fsetter.rsc" "$SYSROOT/opt/GEM/fsetter/de/fsetter.rsc"

	mkdir -p "$SYSROOT/opt/GEM/gemkfat"
	cp "$SRC/tools/mkfatfs/.compile_$TARGET/gemkfatfs.app" "$SYSROOT/opt/GEM/gemkfat/gemkfatfs.app"
	cp "$SRC/tools/mkfatfs/gemkfatfs.rsc" "$SYSROOT/opt/GEM/gemkfat/gemkfatfs.rsc"

	mkdir -p "$SYSROOT/opt/GEM/gluestik"
	cp "$SRC/tools/gluestik/.compile_$TARGET/gluestik.prg" "$SYSROOT/opt/GEM/gluestik"
	cp "$SRC/tools/gluestik/LIESMICH" "$SYSROOT/opt/GEM/gluestik"
	cp "$SRC/tools/gluestik/README" "$SYSROOT/opt/GEM/gluestik"

	mkdir -p "$SYSROOT/opt/GEM/mgw"
	cp "$SRC/tools/mgw/.compile_$TARGET/mgw.prg" "$SYSROOT/opt/GEM/mgw"
	cp -r "$SRC/tools/mgw/examples" "$SYSROOT/opt/GEM/mgw"
	cp "$SRC/tools/mgw/LIESMICH" "$SYSROOT/opt/GEM/mgw"
	cp "$SRC/tools/mgw/README" "$SYSROOT/opt/GEM/mgw"

	mkdir -p "$SYSROOT/opt/GEM/nohog2"
	cp "$SRC/tools/nohog2/.compile_$TARGET/nohog2.acc" "$SYSROOT/opt/GEM/nohog2"
	cp "$SRC/tools/nohog2/README" "$SYSROOT/opt/GEM/nohog2"

	mkdir -p "$SYSROOT/bin"
	cp "$SRC/tools/crypto/.compile_$TARGET/crypto" "$SYSROOT/bin/crypto"
	cp "$SRC/tools/fdisk/.compile_$TARGET/fdisk" "$SYSROOT/bin/fdisk"
	cp "$SRC/tools/fdisk/.compile_$TARGET/sfdisk" "$SYSROOT/bin/sfdisk"
	cp "$SRC/tools/lpflush/.compile_$TARGET/lpflush" "$SYSROOT/bin/lpflush"
	cp "$SRC/tools/minix/fsck/.compile_$TARGET/fsck.minix" "$SYSROOT/bin/fsck.minix"
	cp "$SRC/tools/minix/minit/.compile_$TARGET/minit" "$SYSROOT/bin/minit"
	cp "$SRC/tools/minix/tools/.compile_$TARGET/flist" "$SYSROOT/bin/mflist"
	cp "$SRC/tools/minix/tools/.compile_$TARGET/mfsconf" "$SYSROOT/bin/mfsconf"
	cp "$SRC/tools/mkfatfs/.compile_$TARGET/mkfatfs" "$SYSROOT/bin/mkfatfs"
	cp "$SRC/tools/mktbl/.compile_$TARGET/mktbl" "$SYSROOT/bin/mktbl"
	cp "$SRC/tools/net-tools/.compile_$TARGET/arp" "$SYSROOT/bin/arp"
	cp "$SRC/tools/net-tools/.compile_$TARGET/diald" "$SYSROOT/bin/diald"
	cp "$SRC/tools/net-tools/.compile_$TARGET/ifconfig" "$SYSROOT/bin/ifconfig"
	cp "$SRC/tools/net-tools/.compile_$TARGET/iflink" "$SYSROOT/bin/iflink"
	cp "$SRC/tools/net-tools/.compile_$TARGET/ifstats" "$SYSROOT/bin/ifstats"
	cp "$SRC/tools/net-tools/.compile_$TARGET/masqconf" "$SYSROOT/bin/masqconf"
	cp "$SRC/tools/net-tools/.compile_$TARGET/netstat" "$SYSROOT/bin/netstat"
	cp "$SRC/tools/net-tools/.compile_$TARGET/pppconf" "$SYSROOT/bin/pppconf"
	cp "$SRC/tools/net-tools/.compile_$TARGET/route" "$SYSROOT/bin/route"
	cp "$SRC/tools/net-tools/.compile_$TARGET/slattach" "$SYSROOT/bin/slattach"
	cp "$SRC/tools/net-tools/slinkctl/.compile_$TARGET/slinkctl" "$SYSROOT/bin/slinkctl"
	cp "$SRC/tools/nfs/.compile_$TARGET/mount_nfs" "$SYSROOT/bin/mount_nfs"
	cp "$SRC/tools/strace/.compile_$TARGET/strace" "$SYSROOT/bin/strace"
	cp "$SRC/tools/swkbdtbl/.compile_$TARGET/swkbdtbl" "$SYSROOT/bin/swkbdtbl"
	cp "$SRC/tools/sysctl/.compile_$TARGET/sysctl" "$SYSROOT/bin/sysctl"
}

copy_guides() {
	local GUIDESDIR="$1"
	mkdir -p "$GUIDESDIR"
	cp "$SRC/doc/freemint.hyp" "$GUIDESDIR"
	cp "$SRC/doc/freemint.ref" "$GUIDESDIR"
	cp "$SRC/doc/programmer/mint-prg.hyp" "$GUIDESDIR"
	cp "$SRC/doc/programmer/mint-prg.ref" "$GUIDESDIR"
	cp "$SRC/doc/xaaes/xaaes.hyp" "$GUIDESDIR"
	cp "$SRC/doc/xaaes/xaaes.ref" "$GUIDESDIR"
}

create_filesystem() {
	mkdir -p "$SYSROOT"/{bin,etc,root,tmp,var/run}
	local coreutils="cat cp env ln ls md5sum mkdir mv rm"
	local e2fsprogs="e2fsck mke2fs tune2fs"

	if [ "$CPU_TARGET" = "000" ]
	then
		cp "$BASH_DIR/bash000.ttp" "$SYSROOT/bin/bash"
		for exe in $coreutils; do cp "$COREUTILS_DIR/${exe}000.ttp" "$SYSROOT/bin/${exe}"; done
		for exe in $e2fsprogs; do cp "$E2FSPROGS_DIR/${exe}000.ttp" "$SYSROOT/bin/${exe}"; done
	elif [ "$CPU_TARGET" = "col" ]
	then
		cp "$BASH_DIR/bashv4e.ttp" "$SYSROOT/bin/bash"
		for exe in $coreutils; do cp "$COREUTILS_DIR/${exe}v4e.ttp" "$SYSROOT/bin/${exe}"; done
		for exe in $e2fsprogs; do cp "$E2FSPROGS_DIR/${exe}v4e.ttp" "$SYSROOT/bin/${exe}"; done
	else
		# 02060, 030, 040, 060
		cp "$BASH_DIR/bash020.ttp" "$SYSROOT/bin/bash"
		for exe in $coreutils; do cp "$COREUTILS_DIR/${exe}020.ttp" "$SYSROOT/bin/${exe}"; done
		for exe in $e2fsprogs; do cp "$E2FSPROGS_DIR/${exe}020.ttp" "$SYSROOT/bin/${exe}"; done
	fi
	cp "$E2FSPROGS_DIR/mke2fs.conf" "$SYSROOT/etc"

	echo "root:x:0:0::/root:/bin/bash" > "$SYSROOT/etc/passwd"

	echo "PS1='[\\[\\e[31m\\]\\u\\[\\e[m\\]@\\[\\e[32m\\]\\h\\[\\e[m\\] \\W]\\$ '" > "$SYSROOT/etc/profile"
	echo "export PS1" >> "$SYSROOT/etc/profile"
	echo "alias ls='ls --color'" >> "$SYSROOT/etc/profile"

	touch "$SYSROOT/var/run/utmp"

	cp "$SRC/doc/examples/fscheck.sh" "$SYSROOT/bin"
	cp "$SRC/doc/examples/fstab" "$SYSROOT/etc"

	cp -r "$TERADESK_DIR" "$SYSROOT/opt/GEM"
	cp -r "$QED_DIR" "$SYSROOT/opt/GEM"
	cp -r "$COPS_DIR" "$SYSROOT/opt/GEM"
	cp -r "$TOSWIN2_DIR" "$SYSROOT/opt/GEM"
	
	# can't go to copy_guides because that is called for all builds
	local GUIDESDIR="$1"
	mkdir -p "$GUIDESDIR"
	mkdir -p "$GUIDESDIR/de"
	mkdir -p "$GUIDESDIR/fr"
	cp "$QED_DIR/doc"/qed.{hyp,ref} "$GUIDESDIR"
	cp "$TOSWIN2_DIR/doc/de"/toswin2.{hyp,ref} "$GUIDESDIR/de"
	cp "$TOSWIN2_DIR/doc/en"/toswin2.{hyp,ref} "$GUIDESDIR"
	cp "$TOSWIN2_DIR/doc/fr"/toswin2.{hyp,ref} "$GUIDESDIR/fr"

	cp "$DOSFSTOOLS_DIR/sbin/fsck.fat" "$SYSROOT/bin"
	cp "$DOSFSTOOLS_DIR/sbin/fatlabel" "$SYSROOT/bin"
}
