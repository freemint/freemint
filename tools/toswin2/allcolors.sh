#! /bin/sh

#set -x

bold=`tput bold`
dim=`tput dim`
normal=`tput sgr0`
all=`seq 0 9`

for i in $all; do
    eval "setab$i=\"`tput setab $i`\""
    eval "setaf$i=\"`tput setaf $i`\""
done

cat <<EOF
Normal colors
=============
EOF

for bg in $all; do
    eval "echo -n \$setab${bg} Background no. $bg: "

    for fg in $all; do
	eval "echo -n \$setaf${fg}"
	echo -n " fg $fg "
    done
    echo $normal
done

cat <<EOF
Bold
====
EOF

for bg in $all; do
    eval "echo -n \$setab${bg} Background no. $bg: $bold"

    for fg in $all; do
	eval "echo -n \$setaf${fg}"
	echo -n " fg $fg "
    done
    echo $normal
done

cat <<EOF
Dimmed
======
EOF

for bg in $all; do
    eval "echo -n \$setab${bg} Background no. $bg: $dim"

    for fg in $all; do
	eval "echo -n \$setaf${fg}"
	echo -n " fg $fg "
    done
    echo $normal
done

echo -n $normal
