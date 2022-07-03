# use as: . vttusb_targets.sh

if [ "$CPU_TARGET" = "prg" ]
then
	VTTUSB_TARGETS="prg p30 pst"
elif [ "$CPU_TARGET" = "000" ] || [ "$CPU_TARGET" = "030" ]
then
	VTTUSB_TARGETS="$CPU_TARGET"
elif [ "$CPU_TARGET" = "02060" ]
then
	VTTUSB_TARGETS="030"
else
	VTTUSB_TARGETS=""
fi
