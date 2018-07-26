#!/bin/bash

# Choose which executable to run
EXE_VERSION=MMAP		# or "default" or "MMAP_2MB" or "MMAP_1GB"

# The problem size needs to be large enough to give repeatable, near-asymptotic performance
#   Using 24 cores on a single Xeon Platinum 8160, performance is close to asymptotic for
#        sizes of 12000 or larger.  
# The (internal) iteration count should be large enough to distinguish between runs with
#   consistently slow performance and runs with slow performance due to external interference.
#   The default of 12 yields 11 timed iterations, which is usually sufficient to identify
#   systematically slow runs.
# The number of trials will typically be at least 200, since only one of 100-200
#   runs shows a strong snoop filter conflict.  If this is being run on multiple
#   servers, only the aggregate (#nodes * NUMTRIALS) needs to be large.  Most
#   of the results in the paper included 651 trials (21 trials on each of 31 nodes).

SIZE=12000
ITERS=12
NUMTRIALS=200
MAXTRIAL=$(( $NUMTRIALS - 1 ))

# All cores on a single socket of a Xeon Platinum 8160 -- modify if desired
export MKL_NUM_THREADS=24
# Force the threads to spread out as much as possible
export KMP_AFFINITY=scatter

# The "numactl" command is set up to bind the data and executing threads to socket 0,
# but any socket (or multiple sockets) can be used.
for TRIAL in `seq 0 $MAXTRIAL`
do
	LABEL=`printf %.3d $TRIAL`
	# echo $LABEL
	time numactl --membind=0 --cpunodebind=0 ./dgemm_test_${EXE_VERSION}.exe $SIZE $ITERS > log.${EXE_VERSION}.$LABEL
done
