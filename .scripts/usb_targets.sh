# use as: . usb_targets.sh

if [ "$CPU_TARGET" = "prg" ]
then
	USB_TARGETS="prg plm"
else
	USB_TARGETS="$CPU_TARGET"
fi
