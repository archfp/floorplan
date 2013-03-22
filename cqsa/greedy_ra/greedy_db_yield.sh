#!/bin/bash

# check for command line arguments
if [ $# -ne 2 ]; then
    echo "Usage $0 <benchmark> <# s>"
    exit
fi

BM=$1
CFG=$2

PARAM="-y 2e-4 5e-5 -u 0 -i 0 -a 1 -f 100 -s 10000"

./greedy \
    $PARAM \
    -c ../benchmarks/$BM/config/$CFG-s.cfg \
    -n ../benchmarks/$BM/fp/$CFG-s.nets \
    -t ../benchmarks/$BM/$BM.tg \
    -d ../benchmarks/$BM/$CFG-s_yield.db
    