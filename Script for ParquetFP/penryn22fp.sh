#!/bin/bash

NITER=500
TIMEINIT=3e5
#INBASE=penryn22-no
INBASE=penryn22-noar
OUTBASE=penryn22pl

STAMP=`date +%Y-%m-%d_%H.%M.%S`

if [ $# -eq 0 ] ; then
    mkdir results-$STAMP
    OUTDIR=results-$STAMP

    # bin packing
    ParquetFP -n $NITER -timeInit $TIMEINIT -f $INBASE -save $OUTDIR/$OUTBASE \
	| tee $OUTDIR/log.txt
else
    # WL minimization
    WIREWEIGHT=$1
    AREAWEIGHT=$2

    mkdir results-$STAMP-${WIREWEIGHT}_$AREAWEIGHT
    OUTDIR=results-$STAMP-${WIREWEIGHT}_$AREAWEIGHT
    
    ParquetFP -n $NITER -timeInit $TIMEINIT -f $INBASE -save $OUTDIR/$OUTBASE \
	-minWL -wireWeight $WIREWEIGHT -areaWeight $AREAWEIGHT \
	| tee $OUTDIR/log.txt
fi
