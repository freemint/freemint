#! /usr/bin/perl
# genmagic -- dump magics from (crosscompiled...) genmagic executable
#
# This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
# distribution.
#
$f = 'genmagic.ttp';
$sym = '_magics';
$val = 'A28 N';
#	const char name[28];
#	long value;

# nm for ST executables (-> binutils)
# edit
$nm = '/scratch_hp/fnaumann/m68k-atari-mint/bin/tnm';

($npid = open (NM, "$nm $f|")) || die "nm failed";
$search = << ;
while (<NM>) {
	if (/^$sym\\s+([0-9a-fA-F]+)\\s/) {
		\$off = hex (\$1) + 0x1c;
		print STDERR;
		break;
	}
}

eval $search;
close (NM);
die "not found" if (!defined ($off));

open (F, $f) || die "Can't open";
seek (F, $off, 0) || die "Can't lseek";
while (sysread (F, $d, length (pack ($val, 0)))) {
	($name, $valx) = unpack ($val, $d);
	last if !$name;
	print "%define $name $valx\n";
}
close F || die "Can't close";
