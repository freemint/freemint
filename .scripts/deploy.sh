#!/bin/sh -x

UPLOAD_DIR=web196@server43.webgo24.de:/home/www/snapshots

# zip is default
DEPLOY_ARCHIVE="zip"


ARCHIVE_PATH="${DEPLOY_DIR}/${ARCHIVE_NAME}"

if [ -n "${VERSIONED+x}" ]
then
	subdir=bootable
else
	if [ "$CPU_TARGET" = "prg" ]
	then
		subdir=usb4tos
	else
		subdir=cpu
	fi
fi

eval "$(ssh-agent -s)"

upload_file() {
	local from="$1"
	local to="$2"
	for i in 1 2 3
	do
		scp -o "StrictHostKeyChecking no" "$from" "$to"
		[ $? = 0 ] && return 0
		sleep 1
	done
	exit 1
}

PROJECT_DIR="$PROJECT_NAME"
upload_file "$ARCHIVE_PATH" "${UPLOAD_DIR}/${PROJECT_DIR}/${subdir}/${ARCHIVE_NAME}"

echo ${PROJECT_NAME}-${LONG_VERSION} > .latest_version
upload_file .latest_version "${UPLOAD_DIR}/${PROJECT_DIR}/${subdir}/.latest_version"
