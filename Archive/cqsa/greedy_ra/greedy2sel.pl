#!/usr/bin/perl

$outfile="selected.txt";

$infile=$ARGV[0];
open INFILE, "$infile" or die "Cannot open $infile for reading\n";
open OUTFILE, ">$outfile" or die "Cannot open $outfile for writing\n";

while ($line = <INFILE>) {
    if ($line =~ /^\*\*\* (\d+)/) {
	$line =~ s/\*\*\* //g;
	print OUTFILE "$line"
    }
}

close INFILE;
close OUTFILE;
