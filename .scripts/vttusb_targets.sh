# use as: . vttusb_targets.sh

if [ "$CPU_TARGET" = "prg" ]
then
	VTTUSB_TARGETS="prg p30 pst"
elif [ "$CPU_TARGET" = "000" ]
then
	VTTUSB_TARGETS="000 mst"
elif [ "$CPU_TARGET" = "02060" ] || [ "$CPU_TARGET" = "030" ]
then
	VTTUSB_TARGETS="030"
elif [ "$CPU_TARGET" = "deb" ]
then
	VTTUSB_TARGETS="deb"
else
	VTTUSB_TARGETS=""
fi
