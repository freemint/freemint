# use as: . kernel_targets.sh

if [ "$CPU_TARGET" = "000" ]
then
	# both the bootable st_ste archive and the plain 000 CPU
	# archive ship an 030 kernel for accelerator-card users
	# (TF536, PAK030, ...)
	KERNEL_TARGETS="000 030 hat hat030"
elif [ "$CPU_TARGET" = "02060" ]
then
	KERNEL_TARGETS="030 040 060 mil ara hat030"
elif [ "$CPU_TARGET" != "prg" ]
then
	KERNEL_TARGETS="$CPU_TARGET"
fi
