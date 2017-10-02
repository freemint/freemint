#!/bin/sh -e

# Use as ". build.sh"

TMP="$1"
OUT="$2"

if [ -z "$OLD_BUILD" -o "$OLD_BUILD" -eq 0 ]
then
	export PUBLISH_PATH="${PUBLISH_PATH}/new"

	sed -e "s/\"cur\"/\"${SHORT_ID}\"/;" sys/buildinfo/version.h > sys/buildinfo/version.h.tmp && mv sys/buildinfo/version.h.tmp sys/buildinfo/version.h
	cd ..
	cp -r "${PROJECT}" "${PROJECT}-020"
	cp -r "${PROJECT}" "${PROJECT}-col"

	cd "${PROJECT}" && make && cd ..
	cd "${PROJECT}-020/tools" && make CPU=020-60 && cd ../sys/usb/src.km/loader && make CPU=020-60 && cd ../../../../xaaes/src.km/xaloader && make CPU=020-60 && cd ../../../..
	cd "${PROJECT}-col/tools" && make CPU=v4e && cd ../sys/usb/src.km/loader && make CPU=v4e && cd ../../../../xaaes/src.km/xaloader && make CPU=v4e && cd ../../../..

	gcc -I"${PROJECT}/sys" -o /tmp/version "${PROJECT}/.travis/version.c" && VERSION=$(/tmp/version)

	mkdir -p "${TMP}"
	"./${PROJECT}/.travis/prepare-snapshot.sh"     "${PROJECT}"     "${TMP}/mint-${SHORT_ID}"  "${SHORT_ID}" "${VERSION}"
	"./${PROJECT}/.travis/fixup-cpu.sh"      "020" "${PROJECT}-020" "${TMP}/mint-${SHORT_ID}"  "${SHORT_ID}" "${VERSION}"
	"./${PROJECT}/.travis/fixup-cpu.sh"      "col" "${PROJECT}-col" "${TMP}/mint-${SHORT_ID}"  "${SHORT_ID}" "${VERSION}"

	"./${PROJECT}/.travis/prepare-aranym.sh"       "${PROJECT}"     "${TMP}/aranym-${VERSION}" "${SHORT_ID}" "${VERSION}" "${PROJECT}/.travis/aranym"
	"./${PROJECT}/.travis/fixup-aranym.sh"         "${PROJECT}-020" "${TMP}/aranym-${VERSION}" "${SHORT_ID}" "${VERSION}" "${PROJECT}/.travis/aranym"

	find "${TMP}" -type f -perm -a=x -exec m68k-atari-mint-strip -s {} \;
	mkdir -p "${OUT}"

	cd "${TMP}/mint-${SHORT_ID}/000" && zip -r -9 "${OUT}/${PROJECT}-${VERSION}-000.zip" * > /dev/null && cd -
	cd "${TMP}/mint-${SHORT_ID}/020" && zip -r -9 "${OUT}/${PROJECT}-${VERSION}-020.zip" * > /dev/null && cd -
	cd "${TMP}/mint-${SHORT_ID}/col" && zip -r -9 "${OUT}/${PROJECT}-${VERSION}-col.zip" * > /dev/null && cd -
	cd "${TMP}/mint-${SHORT_ID}" && zip -r -9 "${OUT}/${PROJECT}-${VERSION}-usb4tos.zip" "usb4tos" > /dev/null && cd -

	cd "${TMP}" && zip -r -9 "${OUT}/aranym-${VERSION}.zip" "aranym-${VERSION}" > /dev/null && cd -
	
	cd "${PROJECT}"
else
	export PUBLISH_PATH="${PUBLISH_PATH}/old"

	mkdir -p "${OUT}"
	"./.travis/freemint.org/freemint.build" "${OUT}"
fi
