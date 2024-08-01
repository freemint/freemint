#!/bin/bash -eux
# -e: Exit immediately if a command exits with a non-zero status.
# -u: Treat unset variables as an error when substituting.
# -x: Display expanded script commands

TEMP_CPU_TARGET=$CPU_TARGET
case $CPU_TARGET in
	prg) TEMP_CPU_TARGET=000 ;;
	ara) TEMP_CPU_TARGET=040 ;;
	deb) TEMP_CPU_TARGET=02060 ;;
esac

cd .scripts
for package in toswin2 qed cops hypview
do
	unset PACKAGE_VERSION
	PACKAGE_VERSION=$(wget -q -O - "$DOWNLOAD_DIR/${package}/.latest_version")
	wget -q -O temp.zip "$DOWNLOAD_DIR/${package}/${PACKAGE_VERSION}-${TEMP_CPU_TARGET}.zip" && unzip -q temp.zip && rm temp.zip
done

# teradesk is packaged differently
for package in teradesk
do
	unset PACKAGE_VERSION
	PACKAGE_VERSION=$(wget -q -O - "$DOWNLOAD_DIR/${package}/.latest_version")
	wget -q -O temp.zip "$DOWNLOAD_DIR/${package}/${PACKAGE_VERSION}"
	mkdir -p $package
	cd $package
	unzip -q ../temp.zip
	cd ..
	rm temp.zip
done

cd -
