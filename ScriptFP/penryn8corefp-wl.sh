#!/bin/bash

WIREWEIGHT="0 0.2 0.4 0.6 0.8 1"
AREAWEIGHT="0 0.2 0.4 0.6 0.8 1"

for ww in $WIREWEIGHT; do
for aw in $AREAWEIGHT; do

    COMP=`echo "$ww+$aw <= 1" | bc`
    #echo $ww $aw $SUM
    
if [ "$COMP" -eq 1 ]; then
    #echo $COMP $ww $aw
    ./penryn8corefp.sh $ww $aw
fi

done;
done;
