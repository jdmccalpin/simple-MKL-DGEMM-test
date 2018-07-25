#include <stdio.h>
#include <stdlib.h>
#include <math.h>		// for fmin() and fmax()

#if defined (MMAP_2MB) || defined (MMAP_1GB) || defined (MMAP)
#include <sys/mman.h>
#include <linux/mman.h>
#endif

#include "low_overhead_timers.c"

void dgemm(char *transa, char *transb, int *m, int *n, int *k, double *alpha, double *a, int *lda, double *b, int *ldb, double *beta, double *c, int *ldc);

int main(int argc, char* argv[])
{
	double *A;
	double *B;
	double *C;
    unsigned long *tscs;
    double alpha,beta;
	char transa,transb;
	int i,j,k,m,n,niters;
	long ops;
	double walltime;
	double mintime, maxtime, avgtime, vartime;
	float tsc_freq;
#if defined (MMAP_2MB) || defined (MMAP_1GB) || defined (MMAP)
	long arraybytes, arrayalignment;
    int num_large_pages;
#endif

	if (argc >= 2) {
		m = atoi(argv[1]);
	} else {
		m = 1000;
	}
	if (argc >= 3) {
		niters = atoi(argv[2]);
	} else {
		niters = 6;
	}
	k = m;				// no need to bother with matrices that are not square
	n = m;

	transa='N';			// 'N' -> Not transposed, 'T' -> Transposed
	transb='N';			// --> using 'N' on both A and B is probably cheating, but it does not matter for this benchmark
	alpha = 1.0;		// must be non-zero -- this scales the product of the two matrices
	beta = 1.0;			// may be zero -- this scales the initial value of the output matrix

#if defined (MMAP_2MB)
	// ugly code to make sure that the array size for mmap() is an integral multiple of the 2MiB page size
	arraybytes = sizeof(double) * (long) m * (long)m;	// assume 3 square matrices to keep this simple
	num_large_pages = arraybytes / (2097152UL);			// initial estimate -- usually truncated downward
	if ( arraybytes != num_large_pages*2097152UL ) num_large_pages++;		// if not exact, add one more large page
	arraybytes = (size_t) num_large_pages * 2097152UL;
	printf("INFO: Array size (Bytes) %ld -- may be rounded up to next integral number of 2MiB pages\n",arraybytes);
	A = (double*) mmap( NULL, arraybytes, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB, -1, 0 );
	B = (double*) mmap( NULL, arraybytes, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB, -1, 0 );
	C = (double*) mmap( NULL, arraybytes, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB, -1, 0 );
	if ( (A == (void *)(-1)) || (A == (void *)(-1)) || (A == (void *)(-1)) ) {
		fprintf(stderr,"One of the mmap calls failed: A,B,C: %lx %lx %lx\n",A,B,C);
		exit(1);
	}
#elif defined (MMAP_1GB)
	// ugly code to make sure that the array size for mmap() is an integral multiple of the 1GiB page size
	arraybytes = sizeof(double) * (long) m * (long)m;	// assume 3 square matrices to keep this simple
	num_large_pages = arraybytes / (1073741824L);			// initial estimate -- usually truncated downward
	if ( arraybytes != num_large_pages*1073741824L ) num_large_pages++;		// if not exact, add one more large page
	arraybytes = (size_t) num_large_pages * 1073741824L;
	printf("INFO: Array size (Bytes) %ld -- may be rounded up to next integral number of 1GiB pages\n",arraybytes);
	A = (double*) mmap( NULL, arraybytes, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB | MAP_HUGE_1GB, -1, 0 );
	B = (double*) mmap( NULL, arraybytes, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB | MAP_HUGE_1GB, -1, 0 );
	C = (double*) mmap( NULL, arraybytes, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB | MAP_HUGE_1GB, -1, 0 );
	if ( (A == (void *)(-1)) || (A == (void *)(-1)) || (A == (void *)(-1)) ) {
		fprintf(stderr,"One of the mmap calls failed: A,B,C: %lx %lx %lx\n",A,B,C);
		exit(1);
	}
#elif defined (MMAP)
	// ugly code to make sure that the array size for mmap() is an integral multiple of the 4KiB page size
	arraybytes = sizeof(double) * (long) m * (long)m;	// assume 3 square matrices to keep this simple
	num_large_pages = arraybytes / (4096L);			// initial estimate -- usually truncated downward
	if ( arraybytes != num_large_pages*4096L ) num_large_pages++;		// if not exact, add one more large page
	arraybytes = (size_t) num_large_pages * 4096L;
	printf("INFO: Array size (Bytes) %ld -- may be rounded up to next integral number of 4KiB pages\n",arraybytes);
	A = (double*) mmap( NULL, arraybytes, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0 );
	B = (double*) mmap( NULL, arraybytes, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0 );
	C = (double*) mmap( NULL, arraybytes, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0 );
	if ( (A == (void *)(-1)) || (A == (void *)(-1)) || (A == (void *)(-1)) ) {
		fprintf(stderr,"One of the mmap calls failed: A,B,C: %lx %lx %lx\n",A,B,C);
		exit(1);
	}
#else
	A = (double*) malloc(sizeof(double)*m*k);		// keep the correct dimensions for future reference, even though
	B = (double*) malloc(sizeof(double)*k*n);		// I am only working with square matrices here
	C = (double*) malloc(sizeof(double)*m*n);
#endif
	tscs = (unsigned long*) malloc(sizeof(double)*(niters+1));	// one extra TSC value after the final call to DGEMM

	printf("matrix order is %d\n",m);
	printf("niters %d \n",niters);

	printf("DEBUG: initializing arrays\n");
	for (i=0; i<m*k ; i++) A[i] = 2.0;
	for (i=0; i<k*n ; i++) B[i] = 1.0; 
	for (i=0; i<m*n ; i++) C[i] = 0.0;
    for (i=0; i<niters+1; ++i) tscs[i] = 0;

	printf("DEBUG: starting DGEMM tests \n");
    for (i=0; i<niters; ++i) {
		tscs[i] = rdtscp();
        dgemm(&transa, &transb, &m, &n, &k, &alpha, A, &m, B, &k, &beta, C, &m);
    }
	tscs[niters] = rdtscp();

	tsc_freq = get_TSC_frequency();			// from low-overhead-timers package
	ops = 2*(long)m*(long)n*(long)k;		// careful of overflowing int32 variables -- and ignore any lower-order terms
	printf("operation count %lu\n",ops);
	printf("NOTE: the first iteration will be slow due to MKL setup overhead and should be discarded....\n");
	printf("iter seconds gflops\n");
    for (i=0; i<niters; ++i) {
		walltime = (double)(tscs[i+1]-tscs[i])/tsc_freq;
		printf("%d %f %f\n",i,walltime,(double)ops/walltime/1.0e9);
	}

	// compute statistics on execution time excluding the first iteration
	mintime = 1.0e36;
	maxtime = -1.0e36;
	avgtime = 0.0;
    for (i=1; i<niters; ++i) {
		walltime = (double)(tscs[i+1]-tscs[i])/tsc_freq;
		mintime = fmin(mintime,walltime);
		maxtime = fmax(maxtime,walltime);
		avgtime += walltime;
	}
	avgtime = avgtime / (double)(niters-1);
	printf("STATISTICS mintime %f maxtime %f avgtime %f (max/min)-1 %f (max/avg)-1 %f (min/avg)-1 %f\n",
			mintime, maxtime, avgtime, maxtime/mintime-1.0, maxtime/avgtime-1.0, mintime/avgtime-1.0);
	printf("MIN AVG MAX GFLOPS %f %f %f \n",(double)ops/maxtime/1.0e9,(double)ops/walltime/1.0e9,(double)ops/mintime/1.0e9);

	// compute and print average execution time and average performance excluding the first iteration
	// remember to compute the average GFLOPS from the total ops over total time -- NOT from averaging
	// the individual GFLOPS numbers.

	ops *= (niters-1);
	walltime = (double)(tscs[niters]-tscs[1])/tsc_freq;
	printf("AVERAGE_TIME %f \n",walltime/(double)(niters-1));
	printf("AVERAGE_GFLOPS %f \n",(double)ops/walltime/1.0e9);

	// print out starting TSC for each iteration to help align with externally monitored performance counters
    for (i=0; i<niters+1; ++i) {
		printf("iteration %d started at TSC %lu\n",i,tscs[i]);
	}
}
