#!/bin/bash

EXE_VERSION=default
SIZE=12000
ITERS=6

export MKL_NUM_THREADS=24

for TRIAL in `seq 0 199`
do
	LABEL=`printf %.3d $TRIAL`
	# echo $LABEL
	time numactl --membind=0 --cpunodebind=0 ./dgemm_test_${EXE_VERSION}.exe $SIZE $ITERS > log.${EXE_VERSION}.$LABEL
done
