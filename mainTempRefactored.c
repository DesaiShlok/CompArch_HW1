#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <time.h>

#define REPEAT 1000000

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
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fp = fopen(filename, "w");
    }
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
        uint64_t start = rdtsc_start();
        memcpy(buff2, buff1, size);
        uint64_t end = rdtsc_end();

        clflush(buff1);
        clflush(buff2);

        uint64_t diff = end - start;
        totalTicks += diff;
        writeToFile(diff, fileTicks);
    }

    printf("Total ticks: %llu, Average ticks: %llu\n",
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

    printf("Total ns: %llu, Average ns: %llu\n",
           (long long unsigned)totalNs,
           (long long unsigned)(totalNs / REPEAT));
}

int main() {
    const size_t bufSize = 64;
    char *buf1 = malloc(bufSize);
    char *buf2 = malloc(bufSize);

    // Fill buffer
    memset(buf1, '1', bufSize);

    // File names
    const char *fileTicks = "outTicksTempRefactored.txt";
    const char *fileNs = "outNsTempRefactored.txt";

    // Run tests
    memtest(buf1, buf2, bufSize, fileTicks);
    calcTimes(buf2, buf1, bufSize, fileNs);

    free(buf1);
    free(buf2);

    return 0;
}

