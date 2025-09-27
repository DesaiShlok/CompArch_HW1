#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <time.h>
#include <x86intrin.h>
#include <sched.h>
#include <sys/mman.h>// for mlock

//#define _GNU_SOURCE // have added a replacement as a compiler flag in the Makefile

//files added to try preemp_enable, preempt_disable, hardirq modifications
//#include <linux/module.h>
//#include <linux/kernel.h>
//#include <linux/init.h>
//#include <linux/hardirq.h>
//#include <linux/preempt.h>

#define REPEAT 1000000
#define USED_CORE 2

uint64_t cpuFrequencyInHz=0;
static inline float cpuFreqMhz(int coreIndex)
{
	FILE *fp = fopen("/proc/cpuinfo", "r");
	if(!fp){
	printf("couldnt read cpu freq\n");
	return 0;
	}
	char buffer[256];
	int count=0;
	float frequency=0;
	while(fgets(buffer, sizeof(buffer),fp)){
	
		char *tempBuff=strstr(buffer, "cpu MHz");
		if(tempBuff)
		{
			count++;
			printf("core %d readFreqVal:%s\n",count ,tempBuff);
		}
		if (count==coreIndex){
			char *colonStart = strchr(buffer, ':');
			if(colonStart){
				frequency=atof(colonStart+1);
			}
			fclose(fp);
			return frequency;
			break;
		}
	
	}
	fclose(fp);
	return 0;
}

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

static inline void pinProcessToCPU(int8_t cpuNum)
{
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(cpuNum, &mask);  // set mask to run on CPU 2

	pid_t pid = getpid();

    	if (sched_setaffinity(pid, sizeof(mask), &mask) == -1) {
		perror("sched_setaffinity");
    	}
	else
	{
		printf("Set CPU Affinity to Core %d\n",cpuNum);
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
void memtest(char *buff1, char *buff2, size_t size, const char *fileNameMemTest) {
    createOutFile(fileNameMemTest);

    uint64_t totalTicks = 0;
    uint64_t diffInNsFromTicks=0;
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
	diffInNsFromTicks=((diff*1000000000)/cpuFrequencyInHz); 
        totalTicks += diffInNsFromTicks;
	
	//totalTicks +=diff;
        writeToFile(diffInNsFromTicks, fileNameMemTest);
    }

    printf("Buffer size %zu bytes -- Total ticks: %llu, Average ticks: %llu\n",
           size,
           (long long unsigned)totalTicks,
           (long long unsigned)(totalTicks / REPEAT));
}

// Measure time using clock_gettime
void calcTimes(char *buff1, char *buff2, size_t size, const char *fileNameCalcTimes) {
    createOutFile(fileNameCalcTimes);

    uint64_t totalNs = 0;
    for (long rep = 0; rep < REPEAT; rep++) {
        
    	//cpuid(0, 0, &eax, &ebx, &ecx, &edx);//To serialize
	uint64_t start = timeInNanoSec();
        memcpy(buff2, buff1, size);
	int64_t end = timeInNanoSec();
	//cpuid(0, 0, &eax, &ebx, &ecx, &edx);//To Serialize
        clflush(buff1);
        clflush(buff2);

        uint64_t diff = end - start;
        totalNs += diff;
        writeToFile(diff, fileNameCalcTimes);
    }

    printf("Buffer size %zu bytes -- Total ns: %llu, Average ns: %llu\n",
           size,
           (long long unsigned)totalNs,
           (long long unsigned)(totalNs / REPEAT));
}

int main() {
	pinProcessToCPU(USED_CORE);
	cpuFrequencyInHz = (uint64_t)(cpuFreqMhz(USED_CORE)*1000000);

	// Manually define all file names
    	checkInvariantTsc() ;

char *fileNameMemTest[] = {
        "timeUsingMemTest_2_6.txt", "timeUsingMemTest_2_7.txt", "timeUsingMemTest_2_8.txt",
        "timeUsingMemTest_2_9.txt", "timeUsingMemTest_2_10.txt", "timeUsingMemTest_2_11.txt",
        "timeUsingMemTest_2_12.txt", "timeUsingMemTest_2_13.txt", "timeUsingMemTest_2_14.txt",
        "timeUsingMemTest_2_15.txt", "timeUsingMemTest_2_16.txt", "timeUsingMemTest_2_20.txt",
        "timeUsingMemTest_2_21.txt"
};

char *fileNameCalcTimes[] = {
        "timeUsingCalcTimes_2_6.txt", "timeUsingCalcTimes_2_7.txt", "timeUsingCalcTimes_2_8.txt",
        "timeUsingCalcTimes_2_9.txt", "timeUsingCalcTimes_2_10.txt", "timeUsingCalcTimes_2_11.txt",
        "timeUsingCalcTimes_2_12.txt", "timeUsingCalcTimes_2_13.txt", "timeUsingCalcTimes_2_14.txt",
        "timeUsingCalcTimes_2_15.txt", "timeUsingCalcTimes_2_16.txt", "timeUsingCalcTimes_2_20.txt",
        "timeUsingCalcTimes_2_21.txt"
};
    	int sizes[] = {6,7,8,9,10,11,12,13,14,15,16,20,21};
    	int nSizes = 13;

    	for (int i = 0; i < nSizes; i++) {
       		size_t bufSize = 1ULL << sizes[i];
        	char *buf1 = malloc(bufSize);
        	char *buf2 = malloc(bufSize);
		
		if(mlock(buf1, bufSize)==-1)
                {
                        printf("mlock failed!\n");
                }
		if(mlock(buf2, bufSize)==-1)
		{
			printf("mlock failed!\n");
		}

		if (!buf1 || !buf2) {
           		printf("Failed to allocate buffer of size %zu\n", bufSize);
            		exit(1);
        	}

        	memset(buf1, '1', bufSize);
        	memtest(buf1, buf2, bufSize, fileNameMemTest[i]);
        	calcTimes(buf1, buf2, bufSize, fileNameCalcTimes[i]);
		if(mlock(buf1, bufSize)==-1){
                        printf("munlock failed!\n");
                }
                if(mlock(buf2, bufSize)==-1){
                        printf("munlock failed!\n");
                }

        	free(buf1);
        	free(buf2);
    	}

	return 0;
}

