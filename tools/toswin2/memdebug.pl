#! /usr/local/bin/perl -w
#
# memdebug.pl - parse the output of Toswin2 memory debugger.
# Copyright (C) 2001 Guido Flohr <guido@freemint.de>,
# all rights reserved.
#
# $Id$
#
# This is a quick hack that analyzes the Toswin2 memory management.
# If the variable MEMDEBUG is set to yes in the Makefile, then all
# calls to malloc, free & friends a wrapped into functions that
# report the addresses of the allocated/freed blocks plus the
# location from where the function was called.  Output is normally
# logged to "/tmp/debug.txt".
#
# In order to get good results you have to translate Toswin2
# with "-g" and without "-O".
#
# This Perl script scans the output and tries to find some 
# anomalies.  Call it as
#
#	memdebug.pl toswin2.app /tmp/memdebug.txt
#
# The "real" bugs are usually shown in the first or in the last
# few lines.  The main part will mostly be warnings about memory
# leaks (you can grep -v "never freed" memdebug.txt to avoid
# this).
#
# The script is not suitable for general usage.  The format of the
# debugging output is hard-coded and it will probably fail if
# addr2line outputs any errors.  

my $addr2line = "addr2line";

package Dispatcher;

require 5.004;

use strict;
use integer;

use constant HEX_ADDRESS => qr /^0x[0-9A-F]{8}$/;
use constant HEX_NULL => '0x00000000';

sub new {
	bless {
		_blocks => {},
		_diagnostics => '',
		_callers => {},
	}, shift;
}

sub parse_malloc 
{
	my ($self, $line, $lineno) = @_;

	my (undef, $caller, $bytes, $str_bytes, $str_at, $address, $garbage) = 
		split / /, $line; 
   
	chop $caller if defined $caller;
	unless ($caller =~ HEX_ADDRESS &&
			$bytes =~ HEX_ADDRESS &&
			$str_bytes eq 'bytes' &&
			$str_at eq 'at' &&
			$address =~ HEX_ADDRESS &&
			!defined $garbage) {
		print STDERR <<EOF;
warning: $lineno: corrupted input ignored
EOF

	    # Don't return undef, we'll get a warning otherwise.
 	    return 0;
	}

	$self->{_callers}->{$caller} = "address $caller";
	
	if (exists $self->{_blocks}->{$address} && $address ne HEX_NULL) {
		$self->{_diagnostics} .= <<EOF;
$caller: malloc returned address $address multiple times
EOF
    } elsif ($address ne HEX_NULL) {
		$self->{_blocks}->{$address}->{caller} = $caller;
		$self->{_blocks}->{$address}->{bytes} = $bytes;
	}

	return $line;
}

sub parse_realloc 
{
	my ($self, $line, $lineno) = @_;

	my (undef, $caller, $bytes, $str_from, $addr_old, $str_to, 
		$addr_new, $garbage) = split / /, $line; 
    chop $caller if defined $caller;

	unless ($caller =~ HEX_ADDRESS &&
			$bytes =~ HEX_ADDRESS &&
			$str_from eq 'from' &&
			$addr_old =~ HEX_ADDRESS &&
			$str_to eq 'to' &&
			$addr_new =~ HEX_ADDRESS &&
			!defined $garbage) {
		print STDERR <<EOF;
warning: $lineno: corrupted input ignored
EOF

	    # Don't return undef, we'll get a warning otherwise.
 	    return 0;
	}

	$self->{_callers}->{$caller} = "address $caller";
	
	if (exists $self->{_last_realloc}) {
		if ($self->{_last_realloc}->{address} eq $addr_old) {
			delete $self->{_last_realloc};
		} # else ???
		delete $self->{_last_realloc};
	} # else probably reported elsewhere.

	if ($addr_old ne HEX_NULL) {
		if (exists ($self->{_blocks}->{$addr_old}) && $addr_new ne HEX_NULL) {
			delete $self->{_blocks}->{$addr_old};
		}
	}

	if ($addr_new ne HEX_NULL) {
		$self->{_blocks}->{$addr_new}->{caller} = $caller;
		$self->{_blocks}->{$addr_new}->{bytes} = $bytes;
	}

	return $line;
}

sub parse_pre_realloc 
{
	my ($self, $line, $lineno) = @_;

	my (undef, $caller, $str_to, $bytes, $str_bytes, $str_at, $address, 
		$garbage) = split / /, $line; 
    chop $caller if defined $caller;

	unless ($caller =~ HEX_ADDRESS &&
			$str_to eq 'to' &&
			$bytes =~ HEX_ADDRESS &&
			$str_bytes eq 'bytes' &&
			$str_at eq 'at' &&
			$address =~ HEX_ADDRESS &&
			!defined $garbage) {
		print STDERR <<EOF;
warning: $lineno: corrupted input ignored
EOF

	    # Don't return undef, we'll get a warning otherwise.
 	    return 0;
	}

	$self->{_callers}->{$caller} = "address $caller";
	
	my $pre_bytes;
	unless (exists $self->{_blocks}->{$address} && $address ne HEX_NULL) {
		
		$self->{_diagnostics} .= <<EOF;
$caller: $address reallocated but never allocated
EOF
	    $pre_bytes = "(never allocated)";
    } elsif ($address eq HEX_NULL) {
		$pre_bytes = 0;
	} else {
		$pre_bytes = $self->{_blocks}->{$address}->{bytes};
	}

	$self->{_last_realloc} = { 
		address => $address,
		bytes => $bytes,
		pre_bytes => $pre_bytes,
		caller => $caller,
	}; 

	return $line;
}

sub parse_free 
{
	my ($self, $line, $lineno) = @_;

	my (undef, $caller, $str_at, $address, $garbage) = 
		split / /, $line; 
    chop $caller if defined $caller;

	unless ($caller =~ HEX_ADDRESS &&
			$str_at eq 'at' &&
			$address =~ HEX_ADDRESS &&
			!defined $garbage) {
		print STDERR <<EOF;
warning: $lineno: corrupted input ignored
EOF

	    # Don't return undef, we'll get a warning otherwise.
 	    return 0;
	}

	$self->{_callers}->{$caller} = "address $caller";
	
	unless (exists $self->{_blocks}->{$address} && $address ne HEX_NULL) {
		$self->{_diagnostics} .= <<EOF;
$caller: $address freed but never allocated
EOF
    } elsif ($address eq HEX_NULL) {
		$self->{_diagnostics} .= <<EOF;
$caller: NULL pointer freed
EOF
    } else {
		delete $self->{_blocks}->{$address};
	}

	return $line;
}

sub parse_calloc 
{
	my ($self, $line, $lineno) = @_;

	my (undef, $caller, $nmemb, $str_x, $bytes, $str_bytes, $str_at, $address, 
		$garbage) = split / /, $line; 
    chop $caller if defined $caller;

	unless ($caller =~ HEX_ADDRESS &&
			$nmemb =~ HEX_ADDRESS &&
			$str_x eq 'x' &&
			$bytes =~ HEX_ADDRESS &&
			$str_bytes eq 'bytes' &&
			$str_at eq 'at' &&
			$address =~ HEX_ADDRESS &&
			!defined $garbage) {
		print STDERR <<EOF;
warning: $lineno: corrupted input ignored
EOF

	    # Don't return undef, we'll get a warning otherwise.
 	    return 0;
	}

	$self->{_callers}->{$caller} = "address $caller";
	
	if (exists $self->{_blocks}->{$address} && $address ne HEX_NULL) {
		$self->{_diagnostics} .= <<EOF;
$caller: calloc returned address $address multiple times
EOF
    } elsif ($address ne HEX_NULL) {
		$self->{_blocks}->{$address}->{caller} = $caller;
		$self->{_blocks}->{$address}->{bytes} = "$nmemb x $bytes";
	}

	return $line;
}

sub diagnostics {
	my $self = shift;

   while (my ($addr, $block) = each %{$self->{_blocks}}) {
	   my ($bytes, $caller) = ($block->{bytes}, $block->{caller}); 
	   $self->{_diagnostics} .= <<EOF;
$caller: block at address $addr ($bytes bytes) never freed
EOF
	}

	if ($self->{_last_realloc}) {
		my $last_realloc = $self->{_last_realloc};
		my $pre_bytes = $self->{_blocks}->{$last_realloc->{bytes}} ||
			"never allocated before";

		$self->{_diagnostics} .= <<EOF;
========================================================================
$last_realloc->{caller}: probably crashed while trying to reallocate 
  buffer at address $last_realloc->{address} from $last_realloc->{pre_bytes} 
  bytes to $last_realloc->{bytes} bytes
========================================================================
EOF

	}

	return $self->{_diagnostics};
}

sub callers {
	shift->{_callers};
}

1;

package main;

use strict;

my $re_funcs = q{(malloc|realloc|pre_realloc|free|calloc)};
my $dispatcher = Dispatcher->new;

my ($image, $debug_txt, $garbage) = ($ARGV[0], $ARGV[1], $ARGV[2]);

die "usage: $0 PROGRAM_IMAGE MEMDEBUG_TXT\n"
	unless defined $debug_txt && !defined $garbage;

open IN, "<$debug_txt"
	or die "$0: cannot open $debug_txt: $!\n";
	
while (<IN>) {
	chomp;
	print STDERR "warning: $.: ignoring garbage '$_'\n"
		unless s/^$re_funcs: /
			my $method = "parse_$1";
	        $dispatcher->$method ($_, $.)/xe;
}

close IN;

my $callers = $dispatcher->callers;
my @callers = keys %$callers;
my $warnings = $dispatcher->diagnostics;
my $re_hex = qr /0x[0-9A-F]{8}/;
my $caller_string = (join "\n", @callers) . "\n";

open ADDR2LINE, "echo \"$caller_string\" | $addr2line -e $image |"
	or die "$0: cannot exec $addr2line -e $image: $!\n";

while (<ADDR2LINE>) {
	chomp;
	last unless @callers;
	$callers->{shift @callers} = $_;
}

close ADDR2LINE;

$caller_string = join '|', keys %$callers;

$warnings =~ s/^($caller_string):/$callers->{$1} . ':'/gmex;
print $warnings;

__END__

Local Variables:
mode: perl
perl-indent-level: 4
perl-continued-statement-offset: 4
perl-continued-brace-offset: 0
perl-brace-offset: -4
perl-brace-imaginary-offset: 0
perl-label-offset: -4
tab-width: 4
End:                                                                            




