#!/bin/bash -eux
# -e: Exit immediately if a command exits with a non-zero status.
# -u: Treat unset variables as an error when substituting.
# -x: Display expanded script commands

DOWNLOAD_DIR=http://tho-otto.de/snapshots
echo "DOWNLOAD_DIR=$DOWNLOAD_DIR" >> $GITHUB_ENV
SYSROOT_DIR=${SYSROOT_DIR:-"/usr/m68k-atari-mint/sys-root/usr"}

sudo mkdir -p "${SYSROOT_DIR}" && cd "${SYSROOT_DIR}"

for package in $*
do
	wget -q -O - "$DOWNLOAD_DIR/${package}/${package}-latest.tar.bz2" | sudo tar xjf -
done
