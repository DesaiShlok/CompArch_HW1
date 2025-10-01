
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <inttypes.h>
#include <time.h>
#include <x86intrin.h>
#include <sched.h>
#include <sys/mman.h>
#include <immintrin.h>
#define REPEAT 1000000
#define LINE_SIZE 64

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
    for (size_t i = 0; i < size; i += LINE_SIZE) {
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

double same_row() {
   uint64_t start, end, clock_sum = 0;

   //Create buffers
   char *source = (char *)malloc(4096);
   char *sink = (char *)malloc(LINE_SIZE);

   //Create memory addresses in same row
   char *mem1 = source; 
   char *mem2 = source + LINE_SIZE;
   memset(mem1, '1', 64);

   for (long i = 0; i < REPEAT; i++ ){
      //Flush both mem addresses to ensure clean RAM read
      flush_buffer(mem1,64);
      flush_buffer(mem2,64);

      //Get start time
      start = rdtsc_start();
      //Copy memory
      memcpy(sink, mem1, LINE_SIZE);
      memcpy(sink, mem2, LINE_SIZE);
      memory_fence();
      //Get stop time
      end = rdtsc_end();

      //Calculate tics
      clock_sum += (end - start);
   }
   //Deallocate memory
   free(source);
   free(sink);
   return (double)clock_sum / REPEAT;

}

double different_row() {
   uint64_t start, end, clock_sum = 0;

   //Create 2MB buffer to ensure different rows
   char *source = (char *)malloc((16 * 1024 * 1024) + LINE_SIZE);
   char *sink = (char *)malloc(LINE_SIZE);

   //Create memory addresses in different rows
   char *mem1 = source;
   char *mem2 = source + (16 * 1024 * 1024);

   for (long i = 0; i < REPEAT; i++ ){
      //Flush both mem addresses to ensure clean RAM read
      flush_buffer(mem1,64);
      flush_buffer(mem2,64);

      //Get start time
      start = rdtsc_start();
      //Copy memory
      memcpy(sink, mem1, LINE_SIZE);
      memcpy(sink, mem2, LINE_SIZE);
      memory_fence();
      //Get stop time
      end = rdtsc_end();

      //Calculate tics
      clock_sum += (end - start);
   }
   //Deallocate memory
   free(source);
   free(sink);
   return (double)clock_sum / REPEAT;

}

int main(int ac, char **av)
{
   printf("Average Same Row Time:   %.2f ticks\n", same_row());
   printf("Average Different Row Time:  %.2f ticks\n", different_row());
   return 0;
}
