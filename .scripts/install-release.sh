#!/bin/bash -eux
# -e: Exit immediately if a command exits with a non-zero status.
# -u: Treat unset variables as an error when substituting.
# -x: Display expanded script commands

# Unpacks the CPU_TARGET archive of the latest release of each given freemint
# repository into .scripts/<repository>

CURL_HEADER="Authorization: Bearer $GITHUB_TOKEN"

case "$CPU_TARGET" in
	000|prg) CPU=000 ;;
	col) CPU=v4e ;;
	*) CPU=020 ;;
esac

cd .scripts
for repo in "$@"
do
	url=$(curl -s -H "$CURL_HEADER" "https://api.github.com/repos/freemint/$repo/releases/latest" | jq -r '.assets[].browser_download_url' | grep -- "-mint$CPU\.tar\.bz2$")
	mkdir -p "$repo"
	wget -q -O - "$url" | tar xjf - -C "$repo"
done
cd -
