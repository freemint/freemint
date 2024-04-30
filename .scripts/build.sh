#!/bin/bash -ex
# -e: Exit immediately if a command exits with a non-zero status.
# -x: Display expanded script commands

export M68K_ATARI_MINT_CROSS=yes

if test -d "$SCRIPT_DIR/qed"
then
	export LIBCMINI=yes

	. "$SCRIPT_DIR/version.sh"

	ARANYM="$SCRIPT_DIR/aranym"
	export TERADESK_DIR="$SCRIPT_DIR/teradesk"
	export BASH_DIR="$SCRIPT_DIR/bash"
	export COREUTILS_DIR="$SCRIPT_DIR/coreutils"
	export QED_DIR="$SCRIPT_DIR/qed"
	export DOSFSTOOLS_DIR="$SCRIPT_DIR/dosfstools"
	export COPS_DIR="$SCRIPT_DIR/cops"
	export HYPVIEW_DIR="$SCRIPT_DIR/hypview"
	export TOSWIN2_DIR="$SCRIPT_DIR/toswin2"
	TMP="$SCRIPT_DIR/.tmp"

	mkdir -p "${DEPLOY_DIR}"

	if [ "$CPU_TARGET" = "ara" ]
	then
		make
		DST="${TMP}/aranym-${SHORT_VERSION}"
		"$SCRIPT_DIR/prepare-aranym.sh" "${PWD}" "${DST}" "${SHORT_VERSION}" "${ARANYM}"
		find "${DST}" -type f -perm -a=x -exec ${CROSS_TOOL}-strip -s {} \;
		ARCHIVE_NAME="${PROJECT_NAME}-${LONG_VERSION}-040${VERSIONED}.zip"
		LATEST_NAME="${PROJECT_NAME}-latest-040${VERSIONED}.zip"
		cd "${DST}/.." && zip -r -9 "${DEPLOY_DIR}/${ARCHIVE_NAME}" "$(basename ${DST})" && cd -
	elif [ "$CPU_TARGET" = "prg" ]
	then
		cd sys/usb && make && cd -
		cd tools/usbtool && make all && cd -
		DST="${TMP}/usb4tos-${SHORT_VERSION}"
		"$SCRIPT_DIR/prepare-usb4tos.sh" "${PWD}" "${DST}"
		find "${DST}" -type f -perm -a=x -exec ${CROSS_TOOL}-strip -s {} \;
		ARCHIVE_NAME="usb4tos-${LONG_VERSION}.zip"
		LATEST_NAME="usb4tos-latest.zip"
		cd "${DST}/.." && zip -r -9 "${DEPLOY_DIR}/${ARCHIVE_NAME}" "$(basename ${DST})" && cd -
	else
		make
		DST="${TMP}/mint-${SHORT_VERSION}-${CPU_TARGET}"
		"$SCRIPT_DIR/prepare-snapshot.sh" "${PWD}" "${DST}" "${SHORT_VERSION}" "${SHORT_ID}"
		find "${DST}" -type f -perm -a=x -exec ${CROSS_TOOL}-strip -s {} \;
		ARCHIVE_NAME="${PROJECT_NAME}-${LONG_VERSION}-${CPU_TARGET}${VERSIONED}.zip"
		LATEST_NAME="${PROJECT_NAME}-latest-${CPU_TARGET}${VERSIONED}.zip"
		cd "${DST}" && zip -r -9 "${DEPLOY_DIR}/${ARCHIVE_NAME}" * && cd -
	fi
else
	make
	ARCHIVE_NAME=
	LATEST_NAME=
fi
echo "ARCHIVE_NAME=${ARCHIVE_NAME}" >> $GITHUB_ENV
echo "LATEST_NAME=${LATEST_NAME}" >> $GITHUB_ENV
