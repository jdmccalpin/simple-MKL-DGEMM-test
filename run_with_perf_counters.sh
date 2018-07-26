#!/bin/bash

# Choose a version of the DGEMM benchmark code
EXE_VERSION=MMAP		# or "default" or "MMAP_2MB" or "MMAP_1GB"

# Path to the background performance counter monitoring program
PERFCOUNTERDIRECTORY=../Util/periodic-performance-counters

# The problem size needs to be large enough to give repeatable, near-asymptotic performance
#   Using 24 cores on a single Xeon Platinum 8160, performance is close to asymptotic for
#        sizes of 12000 or larger.  
# The (internal) iteration count should be large enough to distinguish between runs with
#   consistently slow performance and runs with slow performance due to external interference.
#
# When using the "periodic-perf-counters" tool to collect performance counter statistics,
#   the combination of SIZE and ITERS should be chosen so that each trial runs for at least
#   100 performance counter samples.  This allows accurate results when computing deltas
#   using the samples nearest the start TSC of iteration 1 (excluding iteration 0) and
#   nearest the end of the final iteration.
#   
# Most of the results in the paper used SIZE=20000 and ITERS=22, giving an execution
#   time of just over 11 seconds per iteration, and about 235 seconds from the start
#   of iteration 1 (skipping iteration 0) to the end of iteration 22.  This allows
#   use of 1 second performance counter sampling with negligible errors due to 
#   lack of precise start/stop synchronization.
#
# The number of trials will typically be at least 200, since only one of 100-200
# runs shows a strong snoop filter conflict.  If this is being run on multiple
# servers, only the aggregate (#nodes * NUMTRIALS) needs to be large.  Most
# of the results in the paper included 651 trials (21 trials on each of 31 nodes).
# 
SIZE=20000
ITERS=22
NUMTRIALS=200
MAXTRIAL=$(( $NUMTRIALS - 1 ))

# for short tests
SIZE=10000
ITERS=6
NUMTRIALS=10
MAXTRIAL=$(( $NUMTRIALS - 1 ))


# Select number of cores to use -- defaults to all available
export MKL_NUM_THREADS=24
# Spread threads across available cores
#   (The "numactl" command below will keep all threads on the same socket.)
export KMP_AFFINITY=scatter

# Pick a logical processor to run the background performance monitoring code.
#   Overhead is reduced if this is an unused core.
#   Pinning to a single logical processor makes it easy to see the overhead
#     in the performance monitoring output for the selected logical processor.
PERFCOUNTERPROC=93

# "perf_counters" uses a bunch of local files, so switch to that directory
# and run the program in its home directory
# Launch in the background and save the PID to send it a signal at job completion.
pushd $PERFCOUNTERDIRECTORY
taskset -c $PERFCOUNTERPROC ./perf_counters &
PERF_COUNT_PID=$!
popd

for TRIAL in `seq 0 $MAXTRIAL`
do
	LABEL=`printf %.3d $TRIAL`
	# echo $LABEL
	time numactl --membind=0 --cpunodebind=0 ./dgemm_test_${EXE_VERSION}.exe $SIZE $ITERS > log.${EXE_VERSION}.$LABEL
done

echo "Benchmark done -- signalling perf_counters to dump its output"
kill -SIGCONT $PERF_COUNT_PID
echo "sleeping for 10 seconds to let perf_counters finish writing its output"
sleep 10
