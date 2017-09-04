copy_auto() {
	AUTODIR="$1"
	mkdir -p "$AUTODIR"
	cp "$SRC/tools/mintload/mintload.prg" "$AUTODIR/mint-$CUR.prg"
}

copy_kernel() {
	MINTDIR="$1"
	mkdir -p "$MINTDIR"
	shift
	for target in $*
	do
		cp "$SRC/sys/.compile_$target"/mint*.prg "$MINTDIR"
	done
	cp "$SRC/doc/examples/mint.cnf" "$MINTDIR"/mint.cnf
	sed -e "s/#GEM=u:\/c\/mint\/xaaes\/xaloader.prg/GEM=u:\/c\/mint\/$VER\/xaaes\/xaloader.prg/;" "$MINTDIR/mint.cnf" > "$MINTDIR/mint.cnf.tmp" && mv "$MINTDIR/mint.cnf.tmp" "$MINTDIR/mint.cnf"
	mkdir -p "$MINTDIR/doc"
	cp "$SRC/COPYING" "$MINTDIR/doc"
	cp "$SRC/COPYING.GPL" "$MINTDIR/doc"
	cp "$SRC/COPYING.LGPL" "$MINTDIR/doc"
	cp "$SRC/COPYING.MiNT" "$MINTDIR/doc"
}

copy_modules() {
	MINTDIR="$1"
	TARGET="$2"
	mkdir -p "$MINTDIR"
	mkdir -p "$MINTDIR/doc"
	cp "$SRC/sys/sockets/.compile_$TARGET/inet4.xdd" "$MINTDIR"
	mkdir -p "$MINTDIR/doc/inet4"
	cp "$SRC/sys/sockets/inet4/BUGS" "$MINTDIR/doc/inet4"
	cp "$SRC/sys/sockets/COPYING" "$MINTDIR/doc/inet4"
	cp "$SRC/sys/sockets/README" "$MINTDIR/doc/inet4"
	cp "$SRC/sys/sockets/README.1ST" "$MINTDIR/doc/inet4"
	cp "$SRC/sys/sockets/README.masquerade" "$MINTDIR/doc/inet4"
	cp "$SRC/sys/sockets/README.masquerade.TL" "$MINTDIR/doc/inet4"
	cp "$SRC/sys/xdd/lp/.compile_$TARGET/lp.xdd" "$MINTDIR"
	cp "$SRC/sys/xdd/xconout2/.compile_$TARGET/xconout2.xdd" "$MINTDIR"
	cp "$SRC/sys/xdd/xconout2/README" "$MINTDIR/doc/xconout2.txt"
	cp "$SRC/sys/xfs/ext2fs/.compile_$TARGET/ext2.xfs" "$MINTDIR"
	cp "$SRC/sys/xfs/ext2fs/Readme" "$MINTDIR/doc/ext2fs.txt"
	cp "$SRC/sys/xfs/minixfs/.compile_$TARGET/minix.xfs" "$MINTDIR/minix.xfx"
	cp "$SRC/sys/xfs/minixfs/README" "$MINTDIR/doc/minix.txt"
	cp "$SRC/sys/xfs/nfs/.compile_$TARGET/nfs.xfs" "$MINTDIR"
	cp "$SRC/sys/xfs/nfs/README" "$MINTDIR/doc/nfs.txt"
}

# modules compatible with all m68k machines (except the FireBee...)
copy_m68k_modules() {
	SYSDIR="$1"
	mkdir -p "$SYSDIR"
	mkdir -p "$SYSDIR/doc"
	cp "$SRC/sys/sockets/xif/asix.xif" "$SYSDIR/asix.xix"
	cp "$SRC/sys/sockets/xif/plip.xif" "$SYSDIR"
	cp "$SRC/sys/sockets/xif/PLIP.txt" "$SYSDIR/doc/plip.txt"
	cp "$SRC/sys/xdd/audio/audiodev.xdd" "$SYSDIR"
	mkdir -p "$SYSDIR/doc/audiodev"
	cp "$SRC/sys/xdd/audio/README" "$SYSDIR/doc/audiodev"
	cp "$SRC/sys/xdd/audio/README.v0.9" "$SYSDIR/doc/audiodev"
	cp "$SRC/sys/xdd/audio/F030.TODO" "$SYSDIR/doc/audiodev"
	cp "$SRC/sys/xdd/flop-raw/flop_raw.xdd" "$SYSDIR"
	cp "$SRC/sys/xdd/flop-raw/README" "$SYSDIR/doc/flop-raw.txt"
}

# mchdir: st, ste, megaste, tt, falcon, milan, hades, ct60, firebee, aranym
copy_st_modules() {
	MCHDIR="$1/st"
	mkdir -p "$MCHDIR"
	mkdir -p "$MCHDIR/doc"
	# TODO: perhaps these four are compatible also with other machines
	#       but they are awfully old and obsolete so let's keep them here
	cp "$SRC/sys/sockets/xif/biodma.xif" "$MCHDIR/biodma.xix"
	cp "$SRC/sys/sockets/xif/BIODMA.txt" "$MCHDIR/doc/biodma.txt"
	cp "$SRC/sys/sockets/xif/de600.xif" "$MCHDIR/de600.xix"
	cp "$SRC/sys/sockets/xif/DE600.txt" "$MCHDIR/doc/de600.txt"
	cp "$SRC/sys/sockets/xif/dial.xif" "$MCHDIR/dial.xix"
	cp "$SRC/sys/sockets/xif/DIAL.txt" "$MCHDIR/doc/dial.txt"
	cp "$SRC/sys/sockets/xif/pamsdma.xif" "$MCHDIR/pamsdma.xix"
	cp "$SRC/sys/sockets/xif/PAMs_DMA.txt" "$MCHDIR/doc/pamsdma.txt"
	
	cp "$SRC/sys/sockets/xif/rtl8012st.xif" "$MCHDIR/rtl8012st.xix"
	cp "$SRC/sys/xdd/mfp/mfp.xdd" "$MCHDIR"
	mkdir -p "$MCHDIR/doc/mfp"
	cp "$SRC/sys/xdd/mfp/LIESMICH" "$MCHDIR/doc/mfp"
	cp "$SRC/sys/xdd/mfp/README" "$MCHDIR/doc/mfp"
	cp "$SRC/sys/xdd/mfp/ports.txt" "$MCHDIR/doc/mfp"
}
copy_megast_modules() {
	MCHDIR="$1/megast"
	mkdir -p "$MCHDIR"
	mkdir -p "$MCHDIR/doc"
	cp "$SRC/sys/sockets/xif/lance.xif" "$MCHDIR/lance.xix"
	cp "$SRC/sys/sockets/xif/rieblmst.xif" "$MCHDIR/rieblmst.xix"
	cp "$SRC/sys/sockets/xif/rieblmst_fast.xif" "$MCHDIR/rieblmst_fast.xix"
	cp "$SRC/sys/sockets/xif/rieblspc.xif" "$MCHDIR/rieblspc.xix"
	cp "$SRC/sys/sockets/xif/rieblspc_fast.xif" "$MCHDIR/rieblspc_fast.xix"
	cp "$SRC/sys/sockets/xif/LANCE.txt" "$MCHDIR/doc/riebl.txt"
	cp "$SRC/sys/sockets/xif/rtl8012st.xif" "$MCHDIR/rtl8012st.xix"
	cp "$SRC/sys/xdd/mfp/mfp.xdd" "$MCHDIR"
	mkdir -p "$MCHDIR/doc/mfp"
	cp "$SRC/sys/xdd/mfp/LIESMICH" "$MCHDIR/doc/mfp"
	cp "$SRC/sys/xdd/mfp/README" "$MCHDIR/doc/mfp"
	cp "$SRC/sys/xdd/mfp/ports.txt" "$MCHDIR/doc/mfp"
}
copy_ste_modules() {
	MCHDIR="$1/ste"
	mkdir -p "$MCHDIR"
	mkdir -p "$MCHDIR/doc"
	cp "$SRC/sys/sockets/xif/asix.xif" "$MCHDIR/asix.xix"
	cp "$SRC/sys/sockets/xif/rtl8012st.xif" "$MCHDIR/rtl8012st.xix"
	cp "$SRC/sys/xdd/mfp/mfp.xdd" "$MCHDIR"
	mkdir -p "$MCHDIR/doc/mfp"
	cp "$SRC/sys/xdd/mfp/LIESMICH" "$MCHDIR/doc/mfp"
	cp "$SRC/sys/xdd/mfp/README" "$MCHDIR/doc/mfp"
	cp "$SRC/sys/xdd/mfp/ports.txt" "$MCHDIR/doc/mfp"
}
copy_megaste_modules() {
	MCHDIR="$1/megaste"
	mkdir -p "$MCHDIR"
	mkdir -p "$MCHDIR/doc"
	cp "$SRC/sys/sockets/xif/asix.xif" "$MCHDIR/asix.xix"
	cp "$SRC/sys/sockets/xif/rieblste.xif" "$MCHDIR/rieblste.xix"
	cp "$SRC/sys/sockets/xif/LANCE.txt" "$MCHDIR/doc/riebl.txt"
	cp "$SRC/sys/sockets/xif/rtl8012st.xif" "$MCHDIR/rtl8012st.xix"
	cp "$SRC/sys/xdd/mfp/mfp.xdd" "$MCHDIR"
	mkdir -p "$MCHDIR/doc/mfp"
	cp "$SRC/sys/xdd/mfp/LIESMICH" "$MCHDIR/doc/mfp"
	cp "$SRC/sys/xdd/mfp/README" "$MCHDIR/doc/mfp"
	cp "$SRC/sys/xdd/mfp/ports.txt" "$MCHDIR/doc/mfp"
	cp "$SRC/sys/xdd/scc/scc.xdd" "$MCHDIR"
	mkdir -p "$MCHDIR/doc/scc"
	cp "$SRC/sys/xdd/scc/LIESMICH" "$MCHDIR/doc/scc"
	cp "$SRC/sys/xdd/scc/README" "$MCHDIR/doc/scc"
	cp "$SRC/sys/xdd/scc/ports.txt" "$MCHDIR/doc/scc"
}
copy_tt_modules() {
	MCHDIR="$1/tt"
	mkdir -p "$MCHDIR"
	mkdir -p "$MCHDIR/doc"
	cp "$SRC/sys/sockets/xif/daynaport/scsilink.xif" "$MCHDIR/scsilink.xix"
	mkdir -p "$MCHDIR/doc/scsilink"
	cp "$SRC/sys/sockets/xif/daynaport/README" "$MCHDIR/doc/scsilink"
	cp "$SRC/sys/sockets/xif/daynaport/scsi_commands.txt" "$MCHDIR/doc/scsilink"
	cp "$SRC/sys/sockets/xif/lance.xif" "$MCHDIR/lance.xix"
	cp "$SRC/sys/sockets/xif/riebltt.xif" "$MCHDIR/riebltt.xix"
	cp "$SRC/sys/sockets/xif/LANCE.txt" "$MCHDIR/doc/riebl.txt"
	cp "$SRC/sys/sockets/xif/rtl8012.xif" "$MCHDIR/rtl8012.xix"
	cp "$SRC/sys/xdd/mfp/mfp.xdd" "$MCHDIR"
	mkdir -p "$MCHDIR/doc/mfp"
	cp "$SRC/sys/xdd/mfp/LIESMICH" "$MCHDIR/doc/mfp"
	cp "$SRC/sys/xdd/mfp/README" "$MCHDIR/doc/mfp"
	cp "$SRC/sys/xdd/mfp/ports.txt" "$MCHDIR/doc/mfp"
	cp "$SRC/sys/xdd/scc/scc.xdd" "$MCHDIR"
	mkdir -p "$MCHDIR/doc/scc"
	cp "$SRC/sys/xdd/scc/LIESMICH" "$MCHDIR/doc/scc"
	cp "$SRC/sys/xdd/scc/README" "$MCHDIR/doc/scc"
	cp "$SRC/sys/xdd/scc/ports.txt" "$MCHDIR/doc/scc"
}
copy_falcon_modules() {
	MCHDIR="$1/falcon"
	mkdir -p "$MCHDIR"
	mkdir -p "$MCHDIR/doc"
	cp "$SRC/sys/sockets/xif/rtl8012.xif" "$MCHDIR/rtl8012.xix"
	cp "$SRC/sys/sockets/xif/daynaport/scsilink.xif" "$MCHDIR/scsilink.xix"
	mkdir -p "$MCHDIR/doc/scsilink"
	cp "$SRC/sys/sockets/xif/daynaport/README" "$MCHDIR/doc/scsilink"
	cp "$SRC/sys/sockets/xif/daynaport/scsi_commands.txt" "$MCHDIR/doc/scsilink"
	cp "$SRC/sys/xdd/dsp56k/dsp56k.xdd" "$MCHDIR"
	cp "$SRC/sys/xdd/dsp56k/README_MiNT" "$MCHDIR/doc/dsp56k.txt"
	cp "$SRC/sys/xdd/scc/scc.xdd" "$MCHDIR"
	mkdir -p "$MCHDIR/doc/scc"
	cp "$SRC/sys/xdd/scc/LIESMICH" "$MCHDIR/doc/scc"
	cp "$SRC/sys/xdd/scc/README" "$MCHDIR/doc/scc"
	cp "$SRC/sys/xdd/scc/ports.txt" "$MCHDIR/doc/scc"
}
copy_milan_modules() {
	MCHDIR="$1/milan"
	mkdir -p "$MCHDIR"
	mkdir -p "$MCHDIR/doc"
	cp "$SRC/sys/xdd/uart/uart.xdd" "$MCHDIR"
	mkdir -p "$MCHDIR/doc/uart"
	cp "$SRC/sys/xdd/uart/LIESMICH" "$MCHDIR/doc/uart"
	cp "$SRC/sys/xdd/uart/README" "$MCHDIR/doc/uart"
	cp "$SRC/sys/xdd/uart/ports.txt" "$MCHDIR/doc/uart"
	cp "$SRC/sys/xdd/mfp/mfp_mil.xdd" "$MCHDIR"
	mkdir -p "$MCHDIR/doc/mfp"
	cp "$SRC/sys/xdd/mfp/LIESMICH" "$MCHDIR/doc/mfp"
	cp "$SRC/sys/xdd/mfp/README" "$MCHDIR/doc/mfp"
	cp "$SRC/sys/xdd/mfp/ports.txt" "$MCHDIR/doc/mfp"
}
copy_hades_modules() {
	MCHDIR="$1/hades"
	mkdir -p "$MCHDIR"
	mkdir -p "$MCHDIR/doc"
	cp "$SRC/sys/xdd/mfp/mfp.xdd" "$MCHDIR"
	mkdir -p "$MCHDIR/doc/mfp"
	cp "$SRC/sys/xdd/mfp/LIESMICH" "$MCHDIR/doc/mfp"
	cp "$SRC/sys/xdd/mfp/README" "$MCHDIR/doc/mfp"
	cp "$SRC/sys/xdd/mfp/ports.txt" "$MCHDIR/doc/mfp"
}
copy_ct60_modules() {
	MCHDIR="$1/ct60"
	mkdir -p "$MCHDIR"
	mkdir -p "$MCHDIR/doc"

	cp -r "$1/falcon"/* "$MCHDIR"

	cp "$SRC/sys/sockets/xif/ethernat/ethernat.xif" "$MCHDIR/ethernat.xix"
	cp "$SRC/sys/sockets/xif/ethernat/README" "$MCHDIR/doc/ethernat.txt"
	cp "$SRC/sys/sockets/xif/svethlana/svethlan.xif" "$MCHDIR/svethlan.xix"
	cp "$SRC/sys/sockets/xif/svethlana/README" "$MCHDIR/doc/svethlan.txt"
}
copy_firebee_modules() {
	MCHDIR="$1/firebee"
	mkdir -p "$MCHDIR"
	mkdir -p "$MCHDIR/doc"
	cp "$SRC/sys/sockets/xif/fec/fec.xif" "$MCHDIR/fec.xif"
	cp "$SRC/sys/sockets/xif/fec/README" "$MCHDIR/doc/fec.txt"
}
copy_aranym_modules() {
	MCHDIR="$1/aranym"
	mkdir -p "$MCHDIR"
	mkdir -p "$MCHDIR/doc"
	cp "$SRC/sys/sockets/xif/nfeth/nfeth.xif" "$MCHDIR"
	cp "$SRC/sys/sockets/xif/nfeth/README" "$MCHDIR/doc/nfeth.txt"
	cp "$SRC/sys/xdd/nfstderr/nfstderr.xdd" "$MCHDIR"
	cp "$SRC/sys/xdd/nfstderr/README" "$MCHDIR/doc/nfstderr.txt"
	cp "$SRC/sys/xfs/aranym/aranym.xfs" "$MCHDIR"
	cp "$SRC/sys/xfs/aranym/README" "$MCHDIR/doc/aranym.txt"
	# not really needed
	cp "$SRC/sys/xfs/hostfs/hostfs.xfs" "$MCHDIR/hostfs.xfx"
	cp "$SRC/sys/xfs/hostfs/README" "$MCHDIR/doc/hostfs.txt"
}

copy_xaloader() {
	XAAESDIR="$1"
	mkdir -p "$XAAESDIR"
	cp "$SRC/xaaes/src.km/xaloader/xaloader.prg" "$XAAESDIR"
}

copy_xaaes() {
	XAAESDIR="$1"
	TARGET="$2"
	if [ "$TARGET" = "col" ]
	then
        KM_TARGET="v4e"
    else
        KM_TARGET="$TARGET"
	fi
	mkdir -p "$XAAESDIR"
	cp "$SRC/xaaes/src.km/xaaes$KM_TARGET.km" "$XAAESDIR/xaaes.km"
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
	cp "$SRC/xaaes/src.km/example.cnf" "$XAAESDIR/xaaes.cnf"
	sed -e "s/^setenv TOSRUN		u:\\\\usr\\\\gem\\\\toswin2\\\\tw-call.app/setenv TOSRUN		u:\\\\c\\\\mint\\\\$VER\\\\sysroot\\\\GEM\\\\toswin2\\\\tw-call.app/;" "$XAAESDIR/xaaes.cnf" > "$XAAESDIR/xaaes.cnf.tmp" && mv "$XAAESDIR/xaaes.cnf.tmp" "$XAAESDIR/xaaes.cnf"
	sed -e "s/^run c:\\\\tools\\\\toswin2.app/run u:\\\\c\\\\mint\\\\$VER\\\\sysroot\\\\GEM\\\\toswin2\\\\toswin2.app/;" "$XAAESDIR/xaaes.cnf" > "$XAAESDIR/xaaes.cnf.tmp" && mv "$XAAESDIR/xaaes.cnf.tmp" "$XAAESDIR/xaaes.cnf"
	cp "$SRC/xaaes/src.km"/xa_help.* "$XAAESDIR"
	cp "$SRC/xaaes/src.km"/*.rsc "$XAAESDIR"
	cp "$SRC/xaaes/src.km"/*.rsl "$XAAESDIR"
}

copy_usbloader() {
	USBDIR="$1"
	mkdir -p "$USBDIR"
	cp "$SRC/sys/usb/src.km/loader/loader.prg" "$USBDIR"
}

copy_usb() {
	USBDIR="$1"
	TARGET="$2"
	mkdir -p "$USBDIR"
	cp "$SRC/sys/usb/src.km/.compile_$TARGET"/*.km "$USBDIR/usb.km"
	cp "$SRC/sys/usb/src.km/ucd/ehci/.compile_$TARGET/ehci.ucd" "$USBDIR"
	cp "$SRC/sys/usb/src.km/ucd/unicorn/.compile_$TARGET/unicorn.ucd" "$USBDIR"
	cp "$SRC/sys/usb/src.km/udd/eth/.compile_$TARGET/eth.udd" "$USBDIR"
	cp "$SRC/sys/usb/src.km/udd/mouse/.compile_$TARGET/mouse.udd" "$USBDIR"
	cp "$SRC/sys/usb/src.km/udd/storage/.compile_$TARGET/storage.udd" "$USBDIR"
}

copy_usb4tos() {
	mkdir -p "$USB4TOSDIR"
	cp "$SRC/sys/usb/src.km/.compile_prg/usb.prg" "$USB4TOSDIR"
	cp "$SRC/sys/usb/src.km/ucd/unicorn/.compile_prg/unicorn.prg" "$USB4TOSDIR"
	cp "$SRC/sys/usb/src.km/udd/eth/.compile_prg/eth.prg" "$USB4TOSDIR"
	cp "$SRC/sys/usb/src.km/udd/mouse/.compile_prg/mouse.prg" "$USB4TOSDIR"
	cp "$SRC/sys/usb/src.km/udd/storage/.compile_prg/storage.prg" "$USB4TOSDIR"
}

copy_fonts() {
	FONTSDIR="$1"
	mkdir -p "$FONTSDIR"
	cp -r "$SRC/fonts"/* "$FONTSDIR"
}

copy_tbl() {
	TBLDIR="$1"
	mkdir -p "$TBLDIR"
	cp -r "$SRC/sys/tbl"/* "$TBLDIR"
}

copy_sysroot() {
	SYSROOT="$1"
	mkdir -p "$SYSROOT/GEM"
	
	mkdir -p "$SYSROOT/GEM/cops"
	cp "$SRC/tools/cops/cops.app" "$SYSROOT/GEM/cops"
	cp "$SRC/tools/cops/cops_de.app" "$SYSROOT/GEM/cops"
	cp "$SRC/tools/cops/cops_fr.app" "$SYSROOT/GEM/cops"
	
	mkdir -p "$SYSROOT/GEM/fsetter"
	cp "$SRC/tools/fsetter/fsetter.app" "$SYSROOT/GEM/fsetter"
	cp "$SRC/tools/fsetter/COPYING" "$SYSROOT/GEM/fsetter"
	cp "$SRC/tools/fsetter/liesmich" "$SYSROOT/GEM/fsetter"
	cp "$SRC/tools/fsetter/readme" "$SYSROOT/GEM/fsetter"
	cp "$SRC/tools/fsetter/history.txt" "$SYSROOT/GEM/fsetter"
	cp "$SRC/tools/fsetter/fsetter_e.rsc" "$SYSROOT/GEM/fsetter/fsetter.rsc"
	mkdir -p "$SYSROOT/GEM/fsetter/de"
	cp "$SRC/tools/fsetter/fsetter.rsc" "$SYSROOT/GEM/fsetter/de/fsetter.rsc"
	
	# TODO: needs proper gemma installation
	#mkdir -p "$SYSROOT/GEM/gemkfat"
	#cp "$SRC/tools/mkfatfs/COPYING" "$SYSROOT/GEM/gemkfat"
	#cp "$SRC/tools/mkfatfs/gemkfatfs.app" "$SYSROOT/GEM/gemkfat/gemkfat.app"
	#cp "$SRC/tools/mkfatfs/gemkfatfs.rsc" "$SYSROOT/GEM/gemkfat/gemkfat.rsc"
	
	mkdir -p "$SYSROOT/GEM/gluestik"
	cp "$SRC/tools/gluestik/gluestik.prg" "$SYSROOT/GEM/gluestik"
	cp "$SRC/tools/gluestik/COPYING" "$SYSROOT/GEM/gluestik"
	cp "$SRC/tools/gluestik/LIESMICH" "$SYSROOT/GEM/gluestik"
	cp "$SRC/tools/gluestik/README" "$SYSROOT/GEM/gluestik"
	
	mkdir -p "$SYSROOT/GEM/hyp_view"
	cp "$SRC/tools/hypview/hyp_view.app" "$SYSROOT/GEM/hyp_view"
	cp -r "$SRC/tools/hypview/doc" "$SYSROOT/GEM/hyp_view"
	mkdir -p "$SYSROOT/GEM/hyp_view/de"
	cp -r "$SRC/tools/hypview/hyp_view/de.rsc" "$SYSROOT/GEM/hyp_view/de/hyp_view.rsc"
	mkdir -p "$SYSROOT/GEM/hyp_view/cs"
	cp -r "$SRC/tools/hypview/hyp_view/cs.rsc" "$SYSROOT/GEM/hyp_view/cs/hyp_view.rsc"
	cp "$SRC/tools/hypview/release"/* "$SYSROOT/GEM/hyp_view"
	cp -r "$SRC/tools/hypview/skins" "$SYSROOT/GEM/hyp_view"
	cp "$SRC/tools/hypview/hyp_view.bgh" "$SYSROOT/GEM/hyp_view"
	cp "$SRC/tools/hypview/hyp_view/en.rsc" "$SYSROOT/GEM/hyp_view/hyp_view.rsc"
	
	mkdir -p "$SYSROOT/GEM/mgw"
	cp "$SRC/tools/mgw/mgw.prg" "$SYSROOT/GEM/mgw"
	cp -r "$SRC/tools/mgw/examples" "$SYSROOT/GEM/mgw"
	cp "$SRC/tools/mgw/COPYING" "$SYSROOT/GEM/mgw"
	cp "$SRC/tools/mgw/LIESMICH" "$SYSROOT/GEM/mgw"
	cp "$SRC/tools/mgw/README" "$SYSROOT/GEM/mgw"
	
	mkdir -p "$SYSROOT/GEM/nohog2"
	cp "$SRC/tools/nohog2/nohog2.acc" "$SYSROOT/GEM/nohog2"
	cp "$SRC/tools/nohog2/COPYING" "$SYSROOT/GEM/nohog2"
	cp "$SRC/tools/nohog2/README" "$SYSROOT/GEM/nohog2"
	
	mkdir -p "$SYSROOT/GEM/toswin2"
	cp "$SRC/tools/toswin2/toswin2.app" "$SYSROOT/GEM/toswin2"
	cp "$SRC/tools/toswin2/tw-call/tw-call.app" "$SYSROOT/GEM/toswin2"
	# TODO: 'doc' need to be compiled as a HYP
	cp -r "$SRC/tools/toswin2/doc" "$SYSROOT/GEM/toswin2"
	cp "$SRC/tools/toswin2/english/toswin2.rsc" "$SYSROOT/GEM/toswin2"
	cp -r "$SRC/tools/toswin2/screenshots" "$SYSROOT/GEM/toswin2"
	cp "$SRC/tools/toswin2/BUGS" "$SYSROOT/GEM/toswin2"
	cp "$SRC/tools/toswin2/FAQ" "$SYSROOT/GEM/toswin2"
	cp "$SRC/tools/toswin2/NEWS" "$SYSROOT/GEM/toswin2"
	mkdir -p "$SYSROOT/GEM/toswin2/de"
	cp "$SRC/tools/toswin2/toswin2.rsc" "$SYSROOT/GEM/toswin2/de"
	cp "$SRC/tools/toswin2/allcolors.sh" "$SYSROOT/GEM/toswin2"
	cp "$SRC/tools/toswin2/twterm.src" "$SYSROOT/GEM/toswin2"
	cp "$SRC/tools/toswin2/README.terminfo" "$SYSROOT/GEM/toswin2"
	cp "$SRC/tools/toswin2/vttest.txt" "$SYSROOT/GEM/toswin2"
	
	mkdir -p "$SYSROOT/GEM/usb"
	cp "$SRC/tools/usb/usb.acc" "$SYSROOT/GEM/usb"

	mkdir -p "$SYSROOT/bin"
	cp "$SRC/tools/crypto/crypto" "$SYSROOT/bin/crypto.ttp"
	cp "$SRC/tools/fdisk/fdisk" "$SYSROOT/bin/fdisk.ttp"
	cp "$SRC/tools/fdisk/sfdisk" "$SYSROOT/bin/sfdisk.ttp"
	cp "$SRC/tools/lpflush/lpflush" "$SYSROOT/bin/lpflush.ttp"
	cp "$SRC/tools/minix/fsck/fsck.minix" "$SYSROOT/bin/mfsck.ttp"
	cp "$SRC/tools/minix/minit/minit" "$SYSROOT/bin/minit.ttp"
	cp "$SRC/tools/minix/tools/flist" "$SYSROOT/bin/mflist.ttp"
	cp "$SRC/tools/minix/tools/mfsconf" "$SYSROOT/bin/mfsconf.ttp"
	cp "$SRC/tools/mkfatfs/mkfatfs" "$SYSROOT/bin/mkfatfs.ttp"
	cp "$SRC/tools/mktbl/mktbl" "$SYSROOT/bin/mktbl.ttp"
	cp "$SRC/tools/net-tools/arp" "$SYSROOT/bin/arp.ttp"
	cp "$SRC/tools/net-tools/diald" "$SYSROOT/bin/diald.ttp"
	cp "$SRC/tools/net-tools/ifconfig" "$SYSROOT/bin/ifconfig.ttp"
	cp "$SRC/tools/net-tools/iflink" "$SYSROOT/bin/iflink.ttp"
	cp "$SRC/tools/net-tools/ifstats" "$SYSROOT/bin/ifstats.ttp"
	cp "$SRC/tools/net-tools/masqconf" "$SYSROOT/bin/masqconf.ttp"
	cp "$SRC/tools/net-tools/netstat" "$SYSROOT/bin/netstat.ttp"
	cp "$SRC/tools/net-tools/pppconf" "$SYSROOT/bin/pppconf.ttp"
	cp "$SRC/tools/net-tools/route" "$SYSROOT/bin/route.ttp"
	cp "$SRC/tools/net-tools/slattach" "$SYSROOT/bin/slattach.ttp"
	cp "$SRC/tools/net-tools/slinkctl/slinkctl" "$SYSROOT/bin/slinkctl.ttp"
	cp "$SRC/tools/nfs/mount_nfs" "$SYSROOT/bin/nfsmount.ttp"
	cp "$SRC/tools/strace/strace" "$SYSROOT/bin/strace.ttp"
	cp "$SRC/tools/swkbdtbl/swkbdtbl" "$SYSROOT/bin/swkbdtbl.ttp"
	cp "$SRC/tools/sysctl/sysctl" "$SYSROOT/bin/sysctl.ttp"
	
	mkdir -p "$SYSROOT/share/man/man8"
	cp "$SRC/tools/fdisk/sfdisk.8" "$SYSROOT/share/man/man8/sfdisk"
	mkdir -p "$SYSROOT/share/doc/sfdisk"
	cp "$SRC/tools/fdisk/sfdisk.examples" "$SYSROOT/share/doc/sfdisk"
	
	mkdir -p "$SYSROOT/share/man/man1"
	cp "$SRC/tools/lpflush/lpflush.1" "$SYSROOT/share/man/man1/lpflush"
	mkdir -p "$SYSROOT/share/doc/lpflush"
	cp "$SRC/tools/lpflush/COPYING" "$SYSROOT/share/doc/lpflush"
	
	mkdir -p "$SYSROOT/share/doc/minix-tools"
	cp "$SRC/tools/minix/COPYING" "$SYSROOT/share/doc/minix-tools"
	cp "$SRC/tools/minix/docs"/*.doc "$SYSROOT/share/doc/minix-tools"
	
	mkdir -p "$SYSROOT/share/doc/mkfatfs"
	cp "$SRC/tools/mkfatfs/COPYING" "$SYSROOT/share/doc/mkfatfs"
	cp "$SRC/tools/mkfatfs/README" "$SYSROOT/share/doc/mkfatfs"
	
	mkdir -p "$SYSROOT/share/doc/mktbl"
	cp "$SRC/tools/mktbl/COPYING" "$SYSROOT/share/doc/mktbl"
	
	mkdir -p "$SYSROOT/share/man/man8"
	cp "$SRC/tools/net-tools/ifconfig.8" "$SYSROOT/share/man/man8/ifconfig"
	cp "$SRC/tools/net-tools/netstat.8" "$SYSROOT/share/man/man8/netstat"
	cp "$SRC/tools/net-tools/route.8" "$SYSROOT/share/man/man8/route"
	mkdir -p "$SYSROOT/share/doc/net-tools"
	cp "$SRC/tools/net-tools/slinkctl/README" "$SYSROOT/share/doc/net-tools/slinkctl.txt"
	cp "$SRC/tools/net-tools/COPYING" "$SYSROOT/share/doc/net-tools"
	
	mkdir -p "$SYSROOT/share/man/man5"
	cp "$SRC/tools/nfs/mtab.5" "$SYSROOT/share/man/man5/mtab"
	mkdir -p "$SYSROOT/share/man/man8"
	cp "$SRC/tools/nfs/mount.8" "$SYSROOT/share/man/man8/mount"
	mkdir -p "$SYSROOT/share/doc/nfs"
	cp "$SRC/tools/nfs/COPYING" "$SYSROOT/share/doc/nfs"
	cp "$SRC/tools/nfs/README" "$SYSROOT/share/doc/nfs"
}
