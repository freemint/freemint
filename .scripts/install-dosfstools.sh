#!/bin/bash -eux
# -e: Exit immediately if a command exits with a non-zero status.
# -u: Treat unset variables as an error when substituting.
# -x: Display expanded script commands

CURL_HEADER="Authorization: Bearer $GITHUB_TOKEN"

if [ "$CPU_TARGET" = "000" -o "$CPU_TARGET" = "prg" ]
then
	DOSFSTOOLS_URL=$(curl -s -H "$CURL_HEADER" https://api.github.com/repos/freemint/dosfstools/releases/latest | jq -r '.assets[].browser_download_url' | grep 'm68000')
elif [ "$CPU_TARGET" = "col" ]
then
	DOSFSTOOLS_URL=$(curl -s -H "$CURL_HEADER" https://api.github.com/repos/freemint/dosfstools/releases/latest | jq -r '.assets[].browser_download_url' | grep '5475')
else
	# 02060, 030, 040, 060
	DOSFSTOOLS_URL=$(curl -s -H "$CURL_HEADER" https://api.github.com/repos/freemint/dosfstools/releases/latest | jq -r '.assets[].browser_download_url' | grep 'm68020-60')
fi

cd .scripts
mkdir dosfstools
wget -q -O - "$DOSFSTOOLS_URL" | tar xjf - -C dosfstools
cd -
