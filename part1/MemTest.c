#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <time.h>
#include <x86intrin.h>
#include <sched.h>
#include <sys/mman.h>
#include <immintrin.h>

#define REPEAT 10000
#define USED_CORE 2
#define CACHE_LINE_SIZE 64

uint64_t cpuFrequencyInMHz=0;
uint64_t mfenceLatency=0;


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
	CPU_SET(cpuNum, &mask);  

	pid_t pid = getpid();

    	if (sched_setaffinity(pid, sizeof(mask), &mask) == -1) {
		perror("sched_setaffinity");
    	}
	else
	{
		printf("Set CPU Affinity to Core %d\n",cpuNum);
	}
}



static inline void clflush(volatile void *p) {
    asm volatile("clflush (%0)" :: "r"(p));
}

static inline void flush_buffer(volatile void *buf, size_t size) {
    for (size_t i = 0; i < size; i += CACHE_LINE_SIZE) {
        clflush((char*)buf + i);
    }
}
static inline void memory_fence(void) {
    asm volatile("mfence" ::: "memory");
}


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


void memtest(char *buff1, char *buff2, size_t size, const char *fileNameTicksUsingMTest, const char *fileNameTimeUsingMTest) {
    uint64_t *resultsTime = malloc(REPEAT * sizeof(uint64_t));
    if (!resultsTime) {
        printf("Failed to allocate memory for resultsTime array.\n");
        return;
    }
    uint64_t *resultsTicks = malloc(REPEAT * sizeof(uint64_t));
    if (!resultsTicks) {
        printf("Failed to allocate memory for resultsTicks array.\n");
        return;
    }

    uint64_t totalTime = 0, totalTicks = 0;

    for (long rep = 0; rep < REPEAT; rep++) {
        flush_buffer(buff1, size);
        flush_buffer(buff2, size);
        memory_fence(); 
        uint64_t start = rdtsc_start();
        memcpy(buff2, buff1, size);
        memory_fence();
        uint64_t end = rdtsc_end();
        
        uint64_t diffTicks = (end - start);// in ticks
        uint64_t diffTime=(uint64_t)((diffTicks*1000)/cpuFrequencyInMHz);// in nanoseconds
      
        resultsTime[rep] = (uint64_t)(diffTicks); // in ticks
        resultsTicks[rep] = (uint64_t)(diffTime); // in nanoseconds
        
        totalTicks += diffTicks;
        totalTime += diffTime;
        
    }

    printf("Buffer size %zu bytes -- Total ticks: %llu, Average ticks: %llu, Total Time %llu ns, Average Time: %llu ns\n",
           size, 
           (unsigned long long)totalTicks, (unsigned long long)(totalTicks / REPEAT),
           (unsigned long long)totalTime, (unsigned long long)(totalTime / REPEAT));

    FILE *fpTicks = fopen(fileNameTicksUsingMTest, "w");
    if (fpTicks) {
        for (long rep = 0; rep < REPEAT; rep++) {
            fprintf(fpTicks, "%" PRIu64 "\n", resultsTicks[rep]);
        }
        fclose(fpTicks);
    }
     FILE *fpTime = fopen(fileNameTimeUsingMTest, "w");
    if (fpTime) {
        for (long rep = 0; rep < REPEAT; rep++) {
            fprintf(fpTime, "%" PRIu64 "\n", resultsTime[rep]);
        }
        fclose(fpTime);
    }

    free(resultsTime);
    free(resultsTicks);
}

int main() {
	pinProcessToCPU(USED_CORE);
    checkInvariantTsc() ;
	cpuFrequencyInMHz = (uint64_t)(cpuFreqMhz(USED_CORE));
	printf("CPU freq: %ld\n",cpuFrequencyInMHz);

	char *fileNameTicksUsingMTest[] = {
        	"ticksUsingMTest_2_6.txt", "ticksUsingMTest_2_7.txt", "ticksUsingMTest_2_8.txt",
        	"ticksUsingMTest_2_9.txt", "ticksUsingMTest_2_10.txt", "ticksUsingMTest_2_11.txt",
        	"ticksUsingMTest_2_12.txt", "ticksUsingMTest_2_13.txt", "ticksUsingMTest_2_14.txt",
        	"ticksUsingMTest_2_15.txt", "ticksUsingMTest_2_16.txt", "ticksUsingMTest_2_20.txt",
        	"ticksUsingMTest_2_21.txt"
	};
    char *fileNameTimeUsingMTest[] = {
        	"timeUsingMTest_2_6.txt", "timeUsingMTest_2_7.txt", "timeUsingMTest_2_8.txt",
        	"timeUsingMTest_2_9.txt", "timeUsingMTest_2_10.txt", "timeUsingMTest_2_11.txt",
        	"timeUsingMTest_2_12.txt", "timeUsingMTest_2_13.txt", "timeUsingMTest_2_14.txt",
        	"timeUsingMTest_2_15.txt", "timeUsingMTest_2_16.txt", "timeUsingMTest_2_20.txt",
        	"timeUsingMTest_2_21.txt"
	};
    int sizes[] = {
        64,        // 2^6
        128,       // 2^7
        256,       // 2^8
        512,       // 2^9
        1024,      // 2^10
        2048,      // 2^11
        4096,      // 2^12
        8192,      // 2^13
        16384,     // 2^14
        32768,     // 2^15
        65536,     // 2^16
        1048576,   // 2^20
        2097152    // 2^21
    };
    int nSizes = 13;

    for (int i = 0; i < nSizes; i++) {
        size_t bufSize = sizes[i];
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
            
        memtest(buf1, buf2, bufSize, fileNameTicksUsingMTest[i],fileNameTimeUsingMTest[i]);
        
        munlock(buf1, bufSize);
        munlock(buf2, bufSize);

        free(buf1);
        free(buf2);
    }

	return 0;
}

