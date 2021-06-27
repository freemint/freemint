# use as: . xaaes_targets.sh

if [ "$CPU_TARGET" = "000" ]
then
	XAAES_TARGETS="000 sto"
elif [ "$CPU_TARGET" = "ara" ]
then
	XAAES_TARGETS="040"
elif [ "$CPU_TARGET" != "prg" ]
then
	XAAES_TARGETS="$CPU_TARGET"
fi
