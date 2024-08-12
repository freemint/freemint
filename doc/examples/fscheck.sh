#!/bin/bash
# Check filesystems on normal boot or after system crash.
#

if [ -f /etc/fastboot ]; then
	echo "Fast boot. Skipping filesystem checks."
	rm -f /etc/fastboot
else
	dosdrives=`grep -v '^#' /etc/fstab | \
	while read drv mntp type owner group perm; do
		if [ "X$type" = Xdos ]; then
			echo "$drv"
		fi
	done`
	ext2drives=`grep -v '^#' /etc/fstab | \
	while read drv mntp type owner group perm; do
		if [ "X$type" = Xext2 ]; then
			echo "$drv"
		fi
	done`
	mfsdrives=`grep -v '^#' /etc/fstab | \
	while read drv mntp type owner group perm; do
		if [ "X$type" = Xminix ]; then
			echo "$drv"
		fi
	done`
	for drv in $dosdrives; do
		echo; echo "Checking FAT/VFAT filesystem on drive $drv"
		echo "with automatic repair, if needed."
		fsck.fat -p $drv:
		if [ $? -ne 0 ]; then
			echo
			echo "Warning: fsck.fat FAILED"
			echo "         Starting single user shell. Fix the"
			echo "         filesystem on drive $drv by running fsck.fat"
			echo "         manually and REBOOT!"
			echo
			bash -si < /dev/console > /dev/console 2>&1
		fi
	done
	for drv in $ext2drives; do
		echo; echo "Checking ext2 filesystem on drive $drv"
		echo "with automatic repair, if needed."
		e2fsck -C 0 -p $drv:
		if [ $? -ge 2 ]; then
			echo
			echo "Warning: e2fsck FAILED ($?)"
			echo "         Starting single user shell. Fix the"
			echo "         filesystem on drive $drv by running e2fsck"
			echo "         manually and REBOOT!"
			echo
			bash -si < /dev/console > /dev/console 2>&1
		fi
	done
	for drv in $mfsdrives; do
		echo; echo "Checking minix filesystem on drive $drv"
		echo "with automatic repair, if needed."
		fsck.minix -p $drv:
		if [ $? -ne 0 ]; then
			echo
			echo "Warning: fsck.minix FAILED"
			echo "         Starting single user shell. Fix the"
			echo "         filesystem on drive $drv by running fsck.minix"
			echo "         manually and REBOOT!"
			echo
			bash -si < /dev/console > /dev/console 2>&1
		fi
	done
fi
