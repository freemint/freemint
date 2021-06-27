# use as: . netusbee_targets.sh

if [ "$CPU_TARGET" = "prg" ]
then
	NETUSBEE_TARGETS="prg prg_000"
elif [ "$CPU_TARGET" = "col" ]
then
	NETUSBEE_TARGETS=""
else
	NETUSBEE_TARGETS="$CPU_TARGET"
fi
