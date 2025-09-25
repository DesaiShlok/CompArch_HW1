#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <inttypes.h>
#include <time.h>
#define REPEAT 1000000

///@brief to flush cache lines 
static inline void clflush(volatile void *p)
{
	asm volatile("clflush (%0)" ::"r"(p));
}

///@brief to get ticks
static inline uint64_t rdtsc()
{
	unsigned long a, d;
	asm volatile("rdtsc" : "=a"(a), "=d"(d));
	return a | ((uint64_t)d << 32);
}

///@brief to get ticks with a combined CPUID instruction
static inline uint64_t rdtscp()

///@brief to get time from clock_gettime and convert it to nano seconds
static inline uint64_t timeInNanoSec()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);

	return ((uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec);
}

//@brief to check if a file is already present with the name
int8_t checkForOutFile(char *filename)
{
	FILE *fp = fopen(filename, "r");
	if (fp != NULL)
	{
		fclose(fp);
		printf("file exists\n");
		return 1;
	}
	else
	{
		printf("file doesn't exist\n");
		return -1;
	}
	return 0;
}

//@brief to check for a file and create one if not already present
int8_t createOutFile(char *filename)
{
	if (checkForOutFile(filename) == 1)
	{
	}
	else
	{
		FILE *fp = fopen(filename, "w");
		fclose(fp);
	}
	return 0;
}

//@brief to write ticks to the file with a /n
int8_t writeToFile(uint64_t currTicks, char *filename)
{
	FILE *fp = fopen(filename, "a"); // append mode
	if (fp == NULL)
	{
		perror("fopen failed");
		return -1;
	}

	fprintf(fp, "%" PRIu64 "\n", currTicks);

	fclose(fp);
	return 0;
}


char *file1Ticks = "outTicks.txt", *file2NSec = "outNs.txt";
char *filegetTimeAnalysis="outgetTime.txt";
//@brief 
static inline void calcTimes(char *buff1, char *buff2, int size, uint64_t *averageTimeToCopy, uint64_t *totalTimeForTheOperation)
{
	uint64_t timeStart, timeEnd;
	*averageTimeToCopy = 0;
	for (int i = 0; i < REPEAT; ++i)
	{
		uint64_t timeStartNs = timeInNanoSec();
		memcpy(buff2, buff1, size);
		uint64_t timeEndNs = timeInNanoSec();
		
		clflush(buff1);
		clflush(buff2);
		
		uint64_t nanoSecForOp = timeEndNs - timeStartNs;
		
		writeToFile(nanoSecForOp, file2NSec);
		*totalTimeForTheOperation = *totalTimeForTheOperation + nanoSecForOp;

		uint64_t timeFunc=0;
		struct timespec ts;
        	uint64_t startTimeFunc=0,EndTimeFunc=0;
		clock_gettime(CLOCK_MONOTONIC, &ts);
        	startTimeFunc=((uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec);
		timeFunc=timeInNanoSec();
		clock_gettime(CLOCK_MONOTONIC, &ts);
                EndTimeFunc=((uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec);
		writeToFile((EndTimeFunc-startTimeFunc),filegetTimeAnalysis);
	}
	*averageTimeToCopy = *totalTimeForTheOperation / REPEAT;
	printf("The Average time is: %ld\ttotalTimeTaken:%ld\n", *averageTimeToCopy, *totalTimeForTheOperation);
}

char lineBuffer[64];
long int rep;
static inline void memtest(long int ticksPSec)
{
	uint64_t start, end, clock;
	char *lineBuffer = (char *)malloc(64);
	char *lineBufferCopy = (char *)malloc(64);

	createOutFile(file1Ticks);
	createOutFile(file2NSec);
	createOutFile(filegetTimeAnalysis);
	for (int i = 0; i < 64; i++)
	{
		lineBuffer[i] = '1';
	}
	
	printf("size of buffer 1: %ld\tsize of buffer 2: %ld\n", sizeof(lineBuffer), sizeof(lineBufferCopy));
	
	clock = 0;//sums up the total clock cycles for all the operations

	for (rep = 0; rep < REPEAT; rep++)
	{
		start = rdtsc();
		memcpy(lineBufferCopy, lineBuffer, 64);
		end = rdtsc();
		clflush(lineBuffer);
		clflush(lineBufferCopy);
		clock = clock + (end - start);
		writeToFile((end-start), file1Ticks);
	}
	
	uint64_t averageTimeForCopy = 0, totalTimeForOps = 0;
	calcTimes(lineBufferCopy, lineBuffer, 64, &averageTimeForCopy, &totalTimeForOps);
	
	printf("Took %llu ticks total\n", (long long unsigned int)clock);
	
	float averageTicks = (clock /(REPEAT* ticksPSec));
	printf("Time in secs based on sysconf(_SC_CLK_TCK):%f\n", averageTicks);
}

int main(int ac, char **av)
{
	long int numOfTicksPerSec = sysconf(_SC_CLK_TCK);
	printf("\nnumber of ticks in a second:%ld\n", numOfTicksPerSec);
	memtest(numOfTicksPerSec);
	return 0;
}
