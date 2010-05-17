#find newest source-file in $2-$N and generate $1 with BDATETIME accordingly

if [ $# -le 1 ]
then
	echo "usage: $0 target file ..."
	exit 1
fi

TARGET=$1
shift

if [ $# -gt 1 ]
then
	FILES=`ls -t $*`
	set $FILES
fi

echo making for: $1

D=`date -r $1 "+%b %e %Y %H:%M"`
echo \#define BDATETIME \"$D\" >$TARGET
