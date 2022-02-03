#!/bin/bash -eux
# -e: Exit immediately if a command exits with a non-zero status.
# -u: Treat unset variables as an error when substituting.
# -x: Display expanded script commands

# UDO is defined as "~/tmp/udo/udo"
UDO=~/tmp/udo/udo
echo "UDO=$UDO" >> $GITHUB_ENV

UDO_VERSION="7.12"

mkdir -p ~/tmp/udo
cd ~/tmp/udo
wget -q -O - "http://www.tho-otto.de/download/udo-${UDO_VERSION}-linux.tar.bz2" | tar xjf -
cd -
