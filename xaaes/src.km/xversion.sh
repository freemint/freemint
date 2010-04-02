#find newest source-file and generate xversion.h with BDATETIME accordingly
HFILE=xversion.h
N=$HFILE
for i in *.c *.h
do
	if [ $i -nt $N ]
	then
		N=$i
		if [ -f $HFILE ]
		then
			break
		fi
	fi
done
echo $i

#set BDATETIME to date of .
#N=.
if [ $N -nt $HFILE ]
then
	D=`date -r $N "+%b %e %Y %H:%M"`
	echo \#define BDATETIME \"$D\" >$HFILE
fi

