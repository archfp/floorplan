#!/bin/bash

#INDIR=results/thesis/yield
INDIR=results/thesis/lifetime

#OUTDIR=~/phd/thesis/data/yield/dfm\&y09/matlab/input
OUTDIR=~/phd/thesis/data/redundancy_allocation/cases09/matlab/input

#PF=""
PF="+mem"
#PF="+mem+proc"
#PF="+mem+proc+2nd"

echo Postfix: $PF

./greedy2matlab.pl $INDIR/mwd_3-s_cqsa$PF.out
cp matlab.txt $OUTDIR/mwd_3-s_cqsa.txt 

./greedy2matlab.pl $INDIR/mwd_4-s_cqsa$PF.out
cp matlab.txt $OUTDIR/mwd_4-s_cqsa.txt

./greedy2matlab.pl $INDIR/cpl1_4-s_cqsa$PF.out
cp matlab.txt $OUTDIR/mpeg_3pc_4-s_cqsa.txt

./greedy2matlab.pl $INDIR/cpl1_5-s_cqsa$PF.out
cp matlab.txt $OUTDIR/mpeg_3pc_5-s_cqsa.txt

./greedy2matlab.pl $INDIR/cpl2_10-s_cqsa$PF.out
cp matlab.txt $OUTDIR/mpeg_cpl2_nd_10-s_cqsa.txt
