#!/bin/bash -eux
# -e: Exit immediately if a command exits with a non-zero status.
# -u: Treat unset variables as an error when substituting.
# -x: Display expanded script commands

(
	cd ..
	mkdir libcmini
	wget -q -O - https://github.com/freemint/libcmini/archive/master.tar.gz | tar xzf - --strip-components=1 -C libcmini
	cd libcmini
	make
)
