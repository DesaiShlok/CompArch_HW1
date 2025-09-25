#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <time.h>
#include <x86intrin.h>

//files added to try preemp_enable, preempt_disable, hardirq modifications
//#include <linux/module.h>
//#include <linux/kernel.h>
//#include <linux/init.h>
//#include <linux/hardirq.h>
//#include <linux/preempt.h>
//#include <linux/sched.h>

#define REPEAT 1000000

static inline void cpuid(uint32_t leaf, uint32_t subleaf,
                         uint32_t *eax, uint32_t *ebx,
                         uint32_t *ecx, uint32_t *edx) {
    __asm__ volatile("cpuid"
                     : "=a"(*eax), "=b"(*ebx),
                       "=c"(*ecx), "=d"(*edx)
                     : "a"(leaf), "c"(subleaf));
}

int checkInvariantTsc() {
    unsigned int eax, ebx, ecx, edx;
    cpuid(0x80000007,0, &eax, &ebx, &ecx, &edx);

    if ((edx >> 8) & 1) {
        printf("CPU supports invariant TSC. Using RDTSC.\n");
        return 1;
    } else {
        printf("CPU does NOT support invariant TSC. Falling back to CLOCK_MONOTONIC_RAW.\n");
        return 0;
    }
}

// Flush cache line
static inline void clflush(volatile void *p) {
    asm volatile("clflush (%0)" :: "r"(p));
}

// Read TSC with serialization
static inline uint64_t rdtsc_start() {
    unsigned int a, d;
    asm volatile("cpuid\n\t"
                 "rdtsc\n\t"
                 : "=a"(a), "=d"(d)
                 : "a"(0)
                 : "%rbx", "%rcx");
    return ((uint64_t)d << 32) | a;
}

static inline uint64_t rdtsc_end() {
    unsigned int a, d;
    asm volatile("rdtscp\n\t"
                 : "=a"(a), "=d"(d)
                 :
                 : "%rcx");
    return ((uint64_t)d << 32) | a;
}

// Get time in nanoseconds
static inline uint64_t timeInNanoSec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec);
}

// File helpers
static void createOutFile(const char *filename) {
    FILE *fp = fopen(filename, "w"); // overwrite old file
    if (fp) fclose(fp);
}

static void writeToFile(uint64_t value, const char *filename) {
    FILE *fp = fopen(filename, "a");
    if (!fp) return;
    fprintf(fp, "%" PRIu64 "\n", value);
    fclose(fp);
}

// Measure TSC ticks
void memtest(char *buff1, char *buff2, size_t size, const char *fileTicks) {
    createOutFile(fileTicks);

    uint64_t totalTicks = 0;
    for (long rep = 0; rep < REPEAT; rep++) {
	
	/*
	unsigned long flags;
        preeempt_disable();
	raw_local_irq_save(flags);
	*/
	uint64_t start = rdtsc_start();
        memcpy(buff2, buff1, size);
        uint64_t end = rdtsc_end();
	/*
	raw_local_irq_restore(flags);
	preempt_enable();
        */
	clflush(buff1);
        clflush(buff2);

        uint64_t diff = end - start;
        totalTicks += diff;
        writeToFile(diff, fileTicks);
    }

    printf("Buffer size %zu bytes -- Total ticks: %llu, Average ticks: %llu\n",
           size,
           (long long unsigned)totalTicks,
           (long long unsigned)(totalTicks / REPEAT));
}

// Measure time using clock_gettime
void calcTimes(char *buff1, char *buff2, size_t size, const char *fileNs) {
    createOutFile(fileNs);

    uint64_t totalNs = 0;
    for (long rep = 0; rep < REPEAT; rep++) {
        uint64_t start = timeInNanoSec();
        memcpy(buff2, buff1, size);
        uint64_t end = timeInNanoSec();

        clflush(buff1);
        clflush(buff2);

        uint64_t diff = end - start;
        totalNs += diff;
        writeToFile(diff, fileNs);
    }

    printf("Buffer size %zu bytes -- Total ns: %llu, Average ns: %llu\n",
           size,
           (long long unsigned)totalNs,
           (long long unsigned)(totalNs / REPEAT));
}

int main() {
    // Manually define all file names
    checkInvariantTsc() ;
	char *fileTicks[] = {
        "outTicks_2_6.txt", "outTicks_2_7.txt", "outTicks_2_8.txt",
        "outTicks_2_9.txt", "outTicks_2_10.txt", "outTicks_2_11.txt",
        "outTicks_2_12.txt", "outTicks_2_13.txt", "outTicks_2_14.txt",
        "outTicks_2_15.txt", "outTicks_2_16.txt", "outTicks_2_20.txt",
        "outTicks_2_21.txt"
    };

    char *fileNs[] = {
        "outNs_2_6.txt", "outNs_2_7.txt", "outNs_2_8.txt",
        "outNs_2_9.txt", "outNs_2_10.txt", "outNs_2_11.txt",
        "outNs_2_12.txt", "outNs_2_13.txt", "outNs_2_14.txt",
        "outNs_2_15.txt", "outNs_2_16.txt", "outNs_2_20.txt",
        "outNs_2_21.txt"
    };

    int sizes[] = {6,7,8,9,10,11,12,13,14,15,16,20,21};
    int nSizes = 13;

    for (int i = 0; i < nSizes; i++) {
        size_t bufSize = 1ULL << sizes[i];
        char *buf1 = malloc(bufSize);
        char *buf2 = malloc(bufSize);
        if (!buf1 || !buf2) {
            printf("Failed to allocate buffer of size %zu\n", bufSize);
            exit(1);
        }

        memset(buf1, '1', bufSize);

        memtest(buf1, buf2, bufSize, fileTicks[i]);
        calcTimes(buf1, buf2, bufSize, fileNs[i]);

        free(buf1);
        free(buf2);
    }

    return 0;
}

