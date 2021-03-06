#!/usr/bin/perl

# preamble
print "set nokey\n";
print "set size ratio -1\n\n";

# output spec
print "set terminal postscript portrait colour solid 8\n";
print "set output \"out.ps\"\n\n";

# print lines
print "plot '-' w l\n";

while ($line = <>) {
    #print $line;
    if ($line =~ /^(\S+)\s+(\S+)\s+(\S+)\s+DIMS = \((\S+), (\S+)\)/) {
        #print "$1, $2, $3, $4, $5\n";
	# $1 -- component name
	$name=$1;

	# $2, $3, == (x, y) door of lower-left corner
	$x0=$2;
	$y0=$3;

	# $4 -- width
	$x1=$x0+$4;
	
	# %5 -- height
	$y1=$y0+$5;

	# print lines
	print "$x0 $y0\n";
	print "$x1 $y0\n";
	print "$x1 $y1\n";
	print "$x0 $y1\n";
	print "$x0 $y0\n";
	print "\n";
    }
}