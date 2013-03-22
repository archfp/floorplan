#!/usr/bin/perl

# preamble
print "set nokey\n";
print "set size ratio -1\n\n";

# output spec
print "set terminal postscript portrait color solid 8\n";
print "set output \"out.ps\"\n\n";

# print lines
print "plot '-' w l\n";

while ($line = <>) {
    #print $line;
    if ($line =~ /^(\S+)\s+(\S+)\s+(\S+)\s+(\S+)\s+(\S+)/) {
	#print "$1, $2, $3, $4, $5\n";
	# $1 -- component name
	$name=$1;

        # $4, $5, -- (x, y) coor of lower-left corner
	$x0=$4;
	$y0=$5;
	
	# $2 -- width
	$x1=$x0+$2;
	
	# $3 -- height
	$y1=$y0+$3;

	# print lines
	print "$x0 $y0\n";
	print "$x1 $y0\n";
	print "$x1 $y1\n";
	print "$x0 $y1\n";
	print "$x0 $y0\n";
	print "\n";
    } 
} 

