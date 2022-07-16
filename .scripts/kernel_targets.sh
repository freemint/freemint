# use as: . kernel_targets.sh

if [ "$CPU_TARGET" = "000" ]
then
	KERNEL_TARGETS="000 hat"
elif [ "$CPU_TARGET" = "02060" ]
then
	KERNEL_TARGETS="020 030 040 060 mil ara hat030"
elif [ "$CPU_TARGET" = "030" ]
then
	KERNEL_TARGETS="030 hat030"
elif [ "$CPU_TARGET" = "040" ]
then
	KERNEL_TARGETS="040 mil ara"
elif [ "$CPU_TARGET" = "060" ]
then
	KERNEL_TARGETS="060 mil"
elif [ "$CPU_TARGET" != "prg" ]
then
	KERNEL_TARGETS="$CPU_TARGET"
fi
