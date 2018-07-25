# simple-MKL-DGEMM-test

This project contains a simple benchmark of the single-node DGEMM kernel
from Intel's MKL library.

The Makefile is configured to produce four different executables
from the single source file.  The executables differ only in the
method used to allocate the three arrays used in the DGEMM call.

* `dgemm_test_default.exe` uses `malloc()` to allocate the arrays.
* `dgemm_test_MMAP_1GB.exe`uses `mmap()` to allocate the arrays, using the default page size.
* `dgemm_test_MMAP_2MB.exe`uses `mmap()` to allocate the arrays, using 2MiB hugepages.
* `dgemm_test_MMAP.exe`uses `mmap()` to allocate the arrays, using 1GiB hugepages.

The first two should use Transparent Huge Pages if the system is configured
to use them by default.
The `MMAP_2MB` version requires that the hugepages be pre-allocated.
This is typically done with a command of the form `echo 4096 >/proc/sys/vm/nr_hugepages`
The `MMAP_1GB` version requires that 1 GiB pages be pre-allocated.
This typically requires a reboot, with the kernel command line containing
the requested hugepage size and number of hugepages using options like
`hugepagesz=1G hugepages=32`

The script `run_ensemble.sh` provides a simple way to run any of the binaries
repeatedly, as part of a search for performance variability.
The output files are a bit verbose, but the most important values can
be extracted with a simple command like
`grep ^MIN log.default.*` for the output using the default version
of the code.  This gives the minimum, average, and maximum GFLOPS
for the run (excluding the first iteration).  
Interesting things to look for include 
* Runs with low averages, but good maximum GFLOPS.
  * These have probably run into interference with other processes.
* Runs with low averages, but with small spread across min, avg, max GFLOPS.
  * These may indicate a systematic performance degradation related
    to the set of physical addresses used in the run.

As an example, the command `grep ^MIN ExampleResults/log.default.*`
shows the results from 20 runs.  Sorting by the average value (column 6)
shows `log.default.003` with low min, avg, and max values (suggesting
a systematic slowdown), while `log.default.017` has a min GFLOPS
value that is much lower than the (low) avg and max values.  This suggests
both a systematic performance issue AND an interference problem in the
slowest iteration (iteration number 3).
