if [ $# -ne 1 ]
then
	echo usage: $0 "<target>"
	exit 1
fi
wget -S -N -O $1.xml http://wiki.sparemint.org/index.php/Special:Export/$1
