# use as: . netusbee_targets.sh

if [ "$CPU_TARGET" = "prg" ]
then
	NETUSBEE_TARGETS="prg prg_000"
elif [ "$CPU_TARGET" = "col" ]
then
	NETUSBEE_TARGETS=""
elif [ "$CPU_TARGET" = "ara" ]
then
	NETUSBEE_TARGETS="040"
else
	NETUSBEE_TARGETS="$CPU_TARGET"
fi
