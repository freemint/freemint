# use as: . usb_targets.sh

if [ "$CPU_TARGET" = "prg" ]
then
	USB_TARGETS="prg plm"
elif [ "$CPU_TARGET" = "ara" ]
then
	USB_TARGETS="040"
else
	USB_TARGETS="$CPU_TARGET"
fi
