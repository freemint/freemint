#!/usr/bin/perl
# 
# genmagic -- dump magics from (crosscompiled...) genmagic.o object
# file
# 
# This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
# distribution.
#                                                                              
$f = 'genmagic.o';
$firstsym = 'C_PTRACE';
$val = 'A28 N';
#       const char name[28];
#       long value;                                                            

open(M, $f) || die "open failed";
$offset = 0;
while (sysread(M, $d, length(pack( $val, 0)))) {
    ($name, $valx) = unpack( $val, $d );
    last if ( $name =~ /^$firstsym$/s );
    $offset = ++$offset;
    sysseek(M, $offset, SEEK_SET) || die "can't seek";
}

$name =~ /^$firstsym$/s || die "symbols not found";
sysseek(M, $offset, 0) || die "can't seek";
while (sysread(M, $d, length(pack( "A28 N", 0)))) {
    ($name, $valx) = unpack( "A28 N", $d);
    last if !$name;
    print "#define $name $valx\n";
}
close M || die "Can't close";
