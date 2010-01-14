#find newest source-file and generate xversion.h with BDATETIME accordingly
#N=xversion.h
#for i in *.c *.h
#do
#	if [ $i -nt $N ]
#	then
#		N=$i
#	fi
#done
#echo $N

#set BDATETIME to date of .
N=.
if [ $N -nt xversion.h ]
then
	D=`date -r $N "+%b %e %Y %H:%M"`
	echo \#define BDATETIME \"$D\" >xversion.h
fi

