#!/bin/bash -eux
# -e: Exit immediately if a command exits with a non-zero status.
# -u: Treat unset variables as an error when substituting.
# -x: Display expanded script commands

DOWNLOAD_DIR=http://tho-otto.de/snapshots
echo "DOWNLOAD_DIR=$DOWNLOAD_DIR" >> $GITHUB_ENV
SYSROOT_DIR=${SYSROOT_DIR:-"/usr/${CROSS_TOOL}/sys-root"}

sudo mkdir -p "${SYSROOT_DIR}" && cd "${SYSROOT_DIR}"

toolsuffix=${CROSS_TOOL##*-}

for package in $*
do
	# for gcc and binutils, use stable current, not experimental latest build
	case $package in
	gcc | binutils)
		filename=${package}-${toolsuffix}-current.tar.bz2
		;;
	*)
		filename=${package}-${toolsuffix}-latest.tar.bz2
		;;
	esac
	wget -q -O - "$DOWNLOAD_DIR/${package}/${filename}" | sudo tar xjf -
done
