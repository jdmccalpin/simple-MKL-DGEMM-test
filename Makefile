CC = icc 
CCFLAGS=-sox -qopenmp
LDFLAGS=-mkl

SOURCES=simple_MKL_DGEMM_test.c low_overhead_timers.c

default: dgemm_test_default.exe dgemm_test_MMAP.exe dgemm_test_MMAP_2MB.exe dgemm_test_MMAP_1GB.exe

dgemm_test_default.exe: $(SOURCES)
	$(CC) $(CCFLAGS) $< -o $@ $(LDFLAGS)

dgemm_test_MMAP.exe: $(SOURCES)
	$(CC) -DMMAP $(CCFLAGS) $< -o $@ $(LDFLAGS)

dgemm_test_MMAP_2MB.exe: $(SOURCES)
	$(CC) -DMMAP_2MB $(CCFLAGS) $< -o $@ $(LDFLAGS)

dgemm_test_MMAP_1GB.exe: $(SOURCES)
	$(CC) -DMMAP_1GB $(CCFLAGS) $< -o $@ $(LDFLAGS)


clean:
	rm -f *.o
	rm -f dgemm_test_default.exe dgemm_test_MMAP.exe dgemm_test_MMAP_2MB.exe dgemm_test_MMAP_1GB.exe
