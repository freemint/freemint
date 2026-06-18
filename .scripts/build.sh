#!/bin/bash -ex
# -e: Exit immediately if a command exits with a non-zero status.
# -x: Display expanded script commands

export M68K_ATARI_MINT_CROSS=yes

if test -d "$SCRIPT_DIR/qed"
then
	export LIBCMINI=yes

	. "$SCRIPT_DIR/version.sh"

	ARANYM="$SCRIPT_DIR/aranym"
	export FVDI_DIR="$SCRIPT_DIR/fvdi"
	export TERADESK_DIR="$SCRIPT_DIR/teradesk"
	export BASH_DIR="$SCRIPT_DIR/bash"
	export COREUTILS_DIR="$SCRIPT_DIR/coreutils"
	export E2FSPROGS_DIR="$SCRIPT_DIR/e2fsprogs"
	export PING_DIR="$SCRIPT_DIR/ping"
	export QED_DIR="$SCRIPT_DIR/qed"
	export DOSFSTOOLS_DIR="$SCRIPT_DIR/dosfstools"
	export COPS_DIR="$SCRIPT_DIR/cops"
	export TOSWIN2_DIR="$SCRIPT_DIR/toswin2"
	TMP="$SCRIPT_DIR/.tmp"

	mkdir -p "${DEPLOY_DIR}"

	# bootable archives (VERSIONED set, e.g. -st_ste, -tt_falcon_clones,
	# -firebee, -aranym) are CPU-agnostic in name; CPU builds keep the
	# CPU indicator.
	if [ -n "${VERSIONED+x}" ]
	then
		ARCHIVE_NAME="${PROJECT_NAME}-${LONG_VERSION}${VERSIONED}.zip"
		LATEST_NAME="${PROJECT_NAME}-latest${VERSIONED}.zip"
	else
		ARCHIVE_NAME="${PROJECT_NAME}-${LONG_VERSION}-${CPU_TARGET}.zip"
		LATEST_NAME="${PROJECT_NAME}-latest-${CPU_TARGET}.zip"
	fi
	if [ "$CPU_TARGET" = "ara" ]
	then
		make
		DST="${TMP}/aranym-${SHORT_VERSION}"
		"$SCRIPT_DIR/prepare-aranym.sh" "${PWD}" "${DST}" "${SHORT_VERSION}" "${ARANYM}"
		find "${DST}" -type f -perm -a=x -exec ${CROSS_TOOL}-strip -s {} \;
		cd "${DST}/.." && zip -r -9 "${DEPLOY_DIR}/${ARCHIVE_NAME}" "$(basename ${DST})" && cd -
	elif [ "$CPU_TARGET" = "prg" ]
	then
		cd sys/usb && make && cd -
		cd tools/usbtool && make all && cd -
		DST="${TMP}/usb4tos.${SHORT_ID}"
		"$SCRIPT_DIR/prepare-usb4tos.sh" "${PWD}" "${DST}"
		find "${DST}" -type f -perm -a=x -exec ${CROSS_TOOL}-strip -s {} \;
		ARCHIVE_NAME="usb4tos-${LONG_VERSION}.zip"
		LATEST_NAME="usb4tos-latest.zip"
		cd "${DST}/.." && zip -r -9 "${DEPLOY_DIR}/${ARCHIVE_NAME}" "$(basename ${DST})" && cd -
	else
		make
		if [ "$CPU_TARGET" = "02060" ]
		then
			make -C sys/arch/040sp
			make -C sys/arch/060sp
		fi
		DST="${TMP}/mint-${SHORT_VERSION}-${CPU_TARGET}"
		"$SCRIPT_DIR/prepare-snapshot.sh" "${PWD}" "${DST}" "${SHORT_VERSION}" "${SHORT_ID}"
		find "${DST}" -type f -perm -a=x -exec ${CROSS_TOOL}-strip -s {} \;
		cd "${DST}" && zip -r -9 "${DEPLOY_DIR}/${ARCHIVE_NAME}" * && cd -
	fi
else
	make
	ARCHIVE_NAME=
	LATEST_NAME=
fi
echo "ARCHIVE_NAME=${ARCHIVE_NAME}" >> $GITHUB_ENV
echo "LATEST_NAME=${LATEST_NAME}" >> $GITHUB_ENV
