#!/bin/bash -eux
# -e: Exit immediately if a command exits with a non-zero status.
# -u: Treat unset variables as an error when substituting.
# -x: Display expanded script commands

# HCP is defined as "~/tmp/hcp/bin/hcp"
HCP=~/tmp/hcp/bin/hcp
echo "HCP=$HCP" >> $GITHUB_ENV

HCP_VERSION="1.0.4"

mkdir -p ~/tmp
cd ~/tmp
wget -q -O - "http://tho-otto.de/download/hcp-${HCP_VERSION}-linux.tar.bz2" | tar xjf -
mv "hcp-${HCP_VERSION}" hcp
cd -
