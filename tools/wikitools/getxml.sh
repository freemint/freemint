if [ $# -lt 1 ]
then
	echo usage: $0 "<target> [sitename]"
	exit 1
fi
if [ $# -ge 2 ]
then
	SITE=$2
else
	SITE=sparemint.org
fi

wget -S -O $1.xml http://wiki.$SITE/index.php/Special:Export/$1
