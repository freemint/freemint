# use as: . util.sh

copy_auto() {
	local AUTODIR="$1"
	local TARGET="$2"
	local CUR="$3"
	mkdir -p "$AUTODIR"
	cp "$SRC/tools/mintload/.compile_$TARGET/mintload.prg" "$AUTODIR/mint-$CUR.prg"
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

copy_kernel_docs() {
	local MINTDIR="$1"
	local BOOTABLE="$2"
	mkdir -p "$MINTDIR"
	if [ "$BOOTABLE" = "yes" ]
	then
	cp "$SRC/doc/examples/mint.cnf" "$MINTDIR"/mint.cnf
	sed -e "/^#sln e:\/sbin     u:\/sbin/asln c:\/mint\/$VER\/sys-root\/share    u:\/share" "$MINTDIR/mint.cnf" > "$MINTDIR/mint.cnf.tmp" && mv "$MINTDIR/mint.cnf.tmp" "$MINTDIR/mint.cnf"
	sed -e "s/#sln e:\/bin      u:\/bin/sln c:\/mint\/$VER\/sys-root\/bin      u:\/bin/;" "$MINTDIR/mint.cnf" > "$MINTDIR/mint.cnf.tmp" && mv "$MINTDIR/mint.cnf.tmp" "$MINTDIR/mint.cnf"
	sed -e "s/#sln e:\/etc      u:\/etc/sln c:\/mint\/$VER\/sys-root\/etc      u:\/etc/;" "$MINTDIR/mint.cnf" > "$MINTDIR/mint.cnf.tmp" && mv "$MINTDIR/mint.cnf.tmp" "$MINTDIR/mint.cnf"
	sed -e "s/#sln e:\/opt      u:\/opt/sln c:\/mint\/$VER\/sys-root\/opt      u:\/opt/;" "$MINTDIR/mint.cnf" > "$MINTDIR/mint.cnf.tmp" && mv "$MINTDIR/mint.cnf.tmp" "$MINTDIR/mint.cnf"
	sed -e "s/#sln e:\/root     u:\/root/sln c:\/mint\/$VER\/sys-root\/root     u:\/root/;" "$MINTDIR/mint.cnf" > "$MINTDIR/mint.cnf.tmp" && mv "$MINTDIR/mint.cnf.tmp" "$MINTDIR/mint.cnf"
	sed -e "s/#sln e:\/tmp      u:\/tmp/sln c:\/mint\/$VER\/sys-root\/tmp      u:\/tmp/;" "$MINTDIR/mint.cnf" > "$MINTDIR/mint.cnf.tmp" && mv "$MINTDIR/mint.cnf.tmp" "$MINTDIR/mint.cnf"
	sed -e "s/#sln e:\/var      u:\/var/sln c:\/mint\/$VER\/sys-root\/var      u:\/var/;" "$MINTDIR/mint.cnf" > "$MINTDIR/mint.cnf.tmp" && mv "$MINTDIR/mint.cnf.tmp" "$MINTDIR/mint.cnf"
	sed -e "s/#setenv LOGNAME root/setenv LOGNAME root/;" "$MINTDIR/mint.cnf" > "$MINTDIR/mint.cnf.tmp" && mv "$MINTDIR/mint.cnf.tmp" "$MINTDIR/mint.cnf"
	sed -e "s/#setenv USER    root/setenv USER    root/;" "$MINTDIR/mint.cnf" > "$MINTDIR/mint.cnf.tmp" && mv "$MINTDIR/mint.cnf.tmp" "$MINTDIR/mint.cnf"
	sed -e "s/#setenv HOME    \/root/setenv HOME    \/root/;" "$MINTDIR/mint.cnf" > "$MINTDIR/mint.cnf.tmp" && mv "$MINTDIR/mint.cnf.tmp" "$MINTDIR/mint.cnf"
	sed -e "s/#setenv SHELL   \/bin\/bash/setenv SHELL   \/bin\/bash/;" "$MINTDIR/mint.cnf" > "$MINTDIR/mint.cnf.tmp" && mv "$MINTDIR/mint.cnf.tmp" "$MINTDIR/mint.cnf"
	sed -e "s/#GEM=u:\/c\/mint\/xaaes\/xaloader.prg/GEM=u:\/c\/mint\/$VER\/xaaes\/xaloader.prg/;" "$MINTDIR/mint.cnf" > "$MINTDIR/mint.cnf.tmp" && mv "$MINTDIR/mint.cnf.tmp" "$MINTDIR/mint.cnf"
	fi
	mkdir -p "$MINTDIR/doc"
	cp "$SRC/COPYING" "$MINTDIR/doc"
	cp "$SRC/COPYING.GPL" "$MINTDIR/doc"
	cp "$SRC/COPYING.LGPL" "$MINTDIR/doc"
	cp "$SRC/COPYING.MiNT" "$MINTDIR/doc"
}

copy_modules() {
	local MINTDIR="$1"
	local TARGET="$2"
	mkdir -p "$MINTDIR"
	mkdir -p "$MINTDIR/doc"
	
	cp "$SRC/sys/sockets/.compile_$TARGET/inet4.xdd" "$MINTDIR"
	mkdir -p "$MINTDIR/doc/inet4"
	cp "$SRC/sys/sockets/inet4/BUGS" "$MINTDIR/doc/inet4"
	cp "$SRC/sys/sockets/COPYING" "$MINTDIR/doc/inet4"
	cp "$SRC/sys/sockets/README" "$MINTDIR/doc/inet4"
	cp "$SRC/sys/sockets/README.1ST" "$MINTDIR/doc/inet4"
	cp "$SRC/sys/sockets/README.masquerade" "$MINTDIR/doc/inet4/README.mas"
	cp "$SRC/sys/sockets/README.masquerade.TL" "$MINTDIR/doc/inet4/README.mtl"
	
	cp "$SRC/sys/xdd/lp/.compile_$TARGET/lp.xdd" "$MINTDIR"
	
	cp "$SRC/sys/xdd/xconout2/.compile_$TARGET/xconout2.xdd" "$MINTDIR"
	cp "$SRC/sys/xdd/xconout2/README" "$MINTDIR/doc/xconout2.txt"
	
	cp "$SRC/sys/xfs/ext2fs/.compile_$TARGET/ext2.xfs" "$MINTDIR"
	cp "$SRC/sys/xfs/ext2fs/Readme" "$MINTDIR/doc/ext2fs.txt"
	
	cp "$SRC/sys/xfs/minixfs/.compile_$TARGET/minix.xfs" "$MINTDIR/minix.xfx"
	mkdir -p "$MINTDIR/doc/minixfs"
	cp "$SRC/sys/xfs/minixfs/COPYING" "$MINTDIR/doc/minixfs"
	cp "$SRC/sys/xfs/minixfs/README" "$MINTDIR/doc/minixfs"
	
	cp "$SRC/sys/xfs/nfs/.compile_$TARGET/nfs.xfs" "$MINTDIR"
	mkdir -p "$MINTDIR/doc/nfs"
	cp "$SRC/sys/xfs/nfs/COPYING" "$MINTDIR/doc/nfs"
	cp "$SRC/sys/xfs/nfs/README" "$MINTDIR/doc/nfs"
}

# modules compatible with all m68k machines (except the FireBee...)
copy_m68k_modules() {
	local SYSDIR="$1"
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

copy_st_modules() {
	local MCHDIR="$1/st"
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
	local MCHDIR="$1/megast"
	mkdir -p "$MCHDIR"
	mkdir -p "$MCHDIR/doc"
	cp "$SRC/sys/sockets/xif/lance.xif" "$MCHDIR/lance.xix"
	cp "$SRC/sys/sockets/xif/rieblmst.xif" "$MCHDIR/riebl.xix"
	cp "$SRC/sys/sockets/xif/rieblmst_fast.xif" "$MCHDIR/riebl_fast.xix"
	cp "$SRC/sys/sockets/xif/rieblspc.xif" "$MCHDIR/rieblspc.xix"
	cp "$SRC/sys/sockets/xif/rieblspc_fast.xif" "$MCHDIR/riebls_fast.xix"
	cp "$SRC/sys/sockets/xif/LANCE.txt" "$MCHDIR/doc/riebl.txt"
	cp "$SRC/sys/sockets/xif/rtl8012st.xif" "$MCHDIR/rtl8012st.xix"
	cp "$SRC/sys/xdd/mfp/mfp.xdd" "$MCHDIR"
	mkdir -p "$MCHDIR/doc/mfp"
	cp "$SRC/sys/xdd/mfp/LIESMICH" "$MCHDIR/doc/mfp"
	cp "$SRC/sys/xdd/mfp/README" "$MCHDIR/doc/mfp"
	cp "$SRC/sys/xdd/mfp/ports.txt" "$MCHDIR/doc/mfp"
}
copy_ste_modules() {
	local MCHDIR="$1/ste"
	mkdir -p "$MCHDIR"
	mkdir -p "$MCHDIR/doc"
	cp "$SRC/sys/sockets/xif/rtl8012st.xif" "$MCHDIR/rtl8012st.xix"
	cp "$SRC/sys/xdd/mfp/mfp.xdd" "$MCHDIR"
	mkdir -p "$MCHDIR/doc/mfp"
	cp "$SRC/sys/xdd/mfp/LIESMICH" "$MCHDIR/doc/mfp"
	cp "$SRC/sys/xdd/mfp/README" "$MCHDIR/doc/mfp"
	cp "$SRC/sys/xdd/mfp/ports.txt" "$MCHDIR/doc/mfp"
}
copy_megaste_modules() {
	local MCHDIR="$1/megaste"
	mkdir -p "$MCHDIR"
	mkdir -p "$MCHDIR/doc"
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
	local MCHDIR="$1/tt"
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
	local MCHDIR="$1/falcon"
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
	local MCHDIR="$1/milan"
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
	local MCHDIR="$1/hades"
	mkdir -p "$MCHDIR"
	# Hades ESCC is compatible with TT SCC
	mkdir -p "$MCHDIR/doc"
	cp "$SRC/sys/xdd/scc/scc.xdd" "$MCHDIR"
	mkdir -p "$MCHDIR/doc/scc"
	cp "$SRC/sys/xdd/scc/LIESMICH" "$MCHDIR/doc/scc"
	cp "$SRC/sys/xdd/scc/README" "$MCHDIR/doc/scc"
	cp "$SRC/sys/xdd/scc/ports.txt" "$MCHDIR/doc/scc"
}
copy_ct60_modules() {
	local MCHDIR="$1/ct60"
	mkdir -p "$MCHDIR"
	mkdir -p "$MCHDIR/doc"

	cp -r "$1/falcon"/* "$MCHDIR"

	# we don't need plain st/falcon version
	rm "$MCHDIR/rtl8012.xix"
	cp "$SRC/sys/sockets/xif/rtl8012ct60.xif" "$MCHDIR/rtl8012ct60.xix"
	cp "$SRC/sys/sockets/xif/rtl8139.xif" "$MCHDIR/rtl8139.xix"

	cp "$SRC/sys/sockets/xif/ethernat/ethernat.xif" "$MCHDIR/ethernat.xix"
	cp "$SRC/sys/sockets/xif/ethernat/README" "$MCHDIR/doc/ethernat.txt"
	cp "$SRC/sys/sockets/xif/svethlana/svethlan.xif" "$MCHDIR/svethlan.xix"
	cp "$SRC/sys/sockets/xif/svethlana/README" "$MCHDIR/doc/svethlan.txt"
}
copy_firebee_modules() {
	local MCHDIR="$1/firebee"
	mkdir -p "$MCHDIR"
	mkdir -p "$MCHDIR/doc"
	cp "$SRC/sys/sockets/xif/fec/fec.xif" "$MCHDIR/fec.xif"
	cp "$SRC/sys/sockets/xif/fec/README" "$MCHDIR/doc/fec.txt"
}
copy_aranym_modules() {
	local MCHDIR="$1/aranym"
	mkdir -p "$MCHDIR"
	mkdir -p "$MCHDIR/doc"
	cp "$SRC/sys/sockets/xif/nfeth/nfeth.xif" "$MCHDIR"
	cp "$SRC/sys/sockets/xif/nfeth/README" "$MCHDIR/doc/nfeth.txt"
	cp "$SRC/sys/xdd/nfstderr/nfstderr.xdd" "$MCHDIR"
	cp "$SRC/sys/xdd/nfstderr/README" "$MCHDIR/doc/nfstderr.txt"
	cp "$SRC/sys/xdd/nfexec/nfexec.xdd" "$MCHDIR"
	cp "$SRC/sys/xdd/nfexec/README" "$MCHDIR/doc/nfexec.txt"
	cp "$SRC/sys/xfs/aranym/aranym.xfs" "$MCHDIR"
	cp "$SRC/sys/xfs/aranym/README" "$MCHDIR/doc/aranym.txt"
	# not really needed
	cp "$SRC/sys/xfs/hostfs/hostfs.xfs" "$MCHDIR/hostfs.xfx"
	cp "$SRC/sys/xfs/hostfs/README" "$MCHDIR/doc/hostfs.txt"
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
	sed -e "s/^setenv TOSRUN		u:\\\\c\\\\mint\\\\1-19-cur\\\\sys-root\\\\opt\\\\GEM\\\\toswin2\\\\tw-call.app/setenv TOSRUN		u:\\\\c\\\\mint\\\\$VER\\\\sys-root\\\\opt\\\\GEM\\\\toswin2\\\\tw-call.app/;" "$XAAESDIR/xaaes.cnf" > "$XAAESDIR/xaaes.cnf.tmp" && mv "$XAAESDIR/xaaes.cnf.tmp" "$XAAESDIR/xaaes.cnf"
	sed -e "s/^run u:\\\\c\\\\mint\\\\1-19-cur\\\\sys-root\\\\opt\\\\GEM\\\\toswin2\\\\toswin2.app/run u:\\\\c\\\\mint\\\\$VER\\\\sys-root\\\\opt\\\\GEM\\\\toswin2\\\\toswin2.app/;" "$XAAESDIR/xaaes.cnf" > "$XAAESDIR/xaaes.cnf.tmp" && mv "$XAAESDIR/xaaes.cnf.tmp" "$XAAESDIR/xaaes.cnf"
	sed -e "s/^#setenv AVSERVER   \"DESKTOP \"/setenv AVSERVER   \"DESKTOP \"/;" "$XAAESDIR/xaaes.cnf" > "$XAAESDIR/xaaes.cnf.tmp" && mv "$XAAESDIR/xaaes.cnf.tmp" "$XAAESDIR/xaaes.cnf"
	sed -e "s/^#setenv FONTSELECT \"DESKTOP \"/setenv FONTSELECT \"DESKTOP \"/;" "$XAAESDIR/xaaes.cnf" > "$XAAESDIR/xaaes.cnf.tmp" && mv "$XAAESDIR/xaaes.cnf.tmp" "$XAAESDIR/xaaes.cnf"
	if [ "$TARGET" = "col" ]
	then
	sed -e "s/^#shell = c:\\\\teradesk\\\\desktop.prg/shell = u:\\\\c\\\\mint\\\\$VER\\\\sys-root\\\\opt\\\\GEM\\\\teradesk\\\\desk_cf.prg/;" "$XAAESDIR/xaaes.cnf" > "$XAAESDIR/xaaes.cnf.tmp" && mv "$XAAESDIR/xaaes.cnf.tmp" "$XAAESDIR/xaaes.cnf"
	else
	sed -e "s/^#shell = c:\\\\teradesk\\\\desktop.prg/shell = u:\\\\c\\\\mint\\\\$VER\\\\sys-root\\\\opt\\\\GEM\\\\teradesk\\\\desktop.prg/;" "$XAAESDIR/xaaes.cnf" > "$XAAESDIR/xaaes.cnf.tmp" && mv "$XAAESDIR/xaaes.cnf.tmp" "$XAAESDIR/xaaes.cnf"
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

copy_ehci_usb_modules() {
	local USBDIR="$1"
	local TARGET="$2"
	mkdir -p "$USBDIR"
	cp "$SRC/sys/usb/src.km/ucd/ehci/.compile_$TARGET/ehci.ucd" "$USBDIR"
}

copy_ct60_usb_modules() {
	local USBDIR="$1"
	mkdir -p "$USBDIR"
	cp "$SRC/sys/usb/src.km/ucd/ethernat/.compile_060/ethernat.ucd" "$USBDIR"
}

copy_aranym_usb_modules() {
	local USBDIR="$1"
	mkdir -p "$USBDIR"
	cp "$SRC/sys/usb/src.km/ucd/aranym/.compile_040/aranym.ucd" "$USBDIR"
}

copy_usb4tos() {
	local USB4TOSDIR="$1"
	mkdir -p "$USB4TOSDIR"
	cp "$SRC/sys/usb/src.km/.compile_prg/usb.prg" "$USB4TOSDIR"
	cp "$SRC/sys/usb/src.km/ucd/aranym/.compile_prg/aranym.prg" "$USB4TOSDIR"
	cp "$SRC/sys/usb/src.km/ucd/unicorn/.compile_prg/unicorn.prg" "$USB4TOSDIR"
	cp "$SRC/sys/usb/src.km/ucd/netusbee/.compile_prg/netusbee.prg" "$USB4TOSDIR"
	cp "$SRC/sys/usb/src.km/ucd/netusbee/.compile_prg_000/netusbee.prg" "$USB4TOSDIR/netus000.prg"
	cp "$SRC/sys/usb/src.km/ucd/ethernat/.compile_prg/ethernat.prg" "$USB4TOSDIR"
	cp "$SRC/sys/usb/src.km/udd/eth/.compile_prg/eth.prg" "$USB4TOSDIR"
	cp "$SRC/sys/usb/src.km/udd/hid/keyboard/.compile_prg/keyboard.prg" "$USB4TOSDIR"
	cp "$SRC/sys/usb/src.km/udd/hid/mouse/.compile_prg/mouse.prg" "$USB4TOSDIR"
	cp "$SRC/sys/usb/src.km/udd/hid/tablet/.compile_prg/tablet.prg" "$USB4TOSDIR"
	cp "$SRC/sys/usb/src.km/udd/printer/.compile_prg/printer.prg" "$USB4TOSDIR"
	cp "$SRC/sys/usb/src.km/udd/storage/.compile_prg/storage.prg" "$USB4TOSDIR"
	# TODO: multiple CPU variants?
	cp "$SRC/tools/usbtool/.compile_000/usbtool.acc" "$USB4TOSDIR"
}

copy_fonts() {
	local FONTSDIR="$1"
	mkdir -p "$FONTSDIR"
	cp -r "$SRC/fonts"/* "$FONTSDIR"
	# patch long file names
	mv "$FONTSDIR/cs/cp1250_08.txt" "$FONTSDIR/cs/cp125008.txt"
	mv "$FONTSDIR/cs/cp1250_09.txt" "$FONTSDIR/cs/cp125009.txt"
	mv "$FONTSDIR/cs/cp1250_10.txt" "$FONTSDIR/cs/cp125010.txt"
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

	mkdir -p "$SYSROOT/share/man/man8"
	cp "$SRC/tools/fdisk/sfdisk.8" "$SYSROOT/share/man/man8/sfdisk"
	mkdir -p "$SYSROOT/share/doc/sfdisk"
	cp "$SRC/tools/fdisk/sfdisk.examples" "$SYSROOT/share/doc/sfdisk"

	mkdir -p "$SYSROOT/share/man/man1"
	cp "$SRC/tools/lpflush/lpflush.1" "$SYSROOT/share/man/man1/lpflush"
	#mkdir -p "$SYSROOT/share/doc/lpflush"

	mkdir -p "$SYSROOT/share/doc/minix-tools"
	cp "$SRC/tools/minix/COPYING" "$SYSROOT/share/doc/minix-tools"
	cp "$SRC/tools/minix/docs"/*.doc "$SYSROOT/share/doc/minix-tools"

	mkdir -p "$SYSROOT/share/doc/mkfatfs"
	cp "$SRC/tools/mkfatfs/README" "$SYSROOT/share/doc/mkfatfs"

	#mkdir -p "$SYSROOT/share/doc/mktbl"

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

copy_guides() {
	local GUIDESDIR="$1"
	mkdir -p "$GUIDESDIR"
	cp "$SRC/doc/xaaes/xaaes.hyp" "$GUIDESDIR"
	cp "$SRC/doc/xaaes/xaaes.ref" "$GUIDESDIR"
}

create_filesystem() {
	mkdir -p "$SYSROOT"/{bin,etc,root,tmp,var/run}
	local coreutils="cat cp env ln ls md5sum mkdir mv rm"

	if [ "$CPU_TARGET" = "000" ]
	then
		cp "$BASH_DIR/bash000.ttp" "$SYSROOT/bin/bash"
		for exe in $coreutils; do cp "$COREUTILS_DIR/${exe}000.ttp" "$SYSROOT/bin/${exe}"; done
	elif [ "$CPU_TARGET" = "col" ]
	then
		cp "$BASH_DIR/bashv4e.ttp" "$SYSROOT/bin/bash"
		for exe in $coreutils; do cp "$COREUTILS_DIR/${exe}v4e.ttp" "$SYSROOT/bin/${exe}"; done
	else
		# 02060, 030, 040, 060
		cp "$BASH_DIR/bash020.ttp" "$SYSROOT/bin/bash"
		for exe in $coreutils; do cp "$COREUTILS_DIR/${exe}020.ttp" "$SYSROOT/bin/${exe}"; done
	fi

	echo "root:x:0:0::/root:/bin/bash" > "$SYSROOT/etc/passwd"

	echo "PS1='[\\[\\e[31m\\]\\u\\[\\e[m\\]@\\[\\e[32m\\]\\h\\[\\e[m\\] \\W]\\$ '" > "$SYSROOT/etc/profile"
	echo "export PS1" >> "$SYSROOT/etc/profile"
	echo "alias ls='ls --color'" >> "$SYSROOT/etc/profile"

	touch "$SYSROOT/var/run/utmp"

	cp -r "$TERADESK_DIR" "$SYSROOT/opt/GEM"
	cp -r "$QED_DIR" "$SYSROOT/opt/GEM"
	cp -r "$COPS_DIR" "$SYSROOT/opt/GEM"
	cp -r "$HYPVIEW_DIR" "$SYSROOT/opt/GEM"
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
