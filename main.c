#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <inttypes.h>
#include <time.h>
#define REPEAT 1000000

static inline void clflush(volatile void *p)
{
	asm volatile("clflush (%0)" ::"r"(p));
}

static inline uint64_t rdtsc()
{
	unsigned long a, d;
	asm volatile("rdtsc" : "=a"(a), "=d"(d));
	return a | ((uint64_t)d << 32);
}
static inline uint64_t timeInNanoSec()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);

	return ((uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec);
}
int8_t checkForOutFile(char *filename)
{
	FILE *fp = fopen(filename, "r");
	if (fp != NULL)
	{
		fclose(fp);
		printf("file exists");
		return 1;
	}
	else
	{
		printf("file doesn't exist");
		return -1;
	}
	return 0;
}
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

static inline void calcTimes(char *buff1, char *buff2, int size, uint64_t *averageTicksToCopy, uint64_t *totalTimeForTheOperation)
{
	uint64_t timeStart, timeEnd;
	*averageTicksToCopy = 0;
	for (int i = 0; i < REPEAT; ++i)
	{
		uint64_t timeStartNs = timeInNanoSec();
		timeStart = rdtsc();
		memcpy(buff2, buff1, size);
		timeEnd = rdtsc();
		uint64_t timeEndNs = timeInNanoSec();
		clflush(buff1);
		clflush(buff2);
		uint64_t ticksForOp = timeEnd - timeStart;
		uint64_t nanoSecForOp = timeEndNs - timeStartNs;
		writeToFile(ticksForOp, file1Ticks);
		writeToFile(nanoSecForOp, file2NSec);
		*totalTimeForTheOperation = *totalTimeForTheOperation + ticksForOp;
	}
	*averageTicksToCopy = *totalTimeForTheOperation / REPEAT;
	printf("The Average is: %ld\ttotalTimeTaken:%ld\n", *averageTicksToCopy, *totalTimeForTheOperation);
}

char lineBuffer[64];
long int rep;
static inline void memtest(int ticksPSec)
{
	uint64_t start, end, clock;
	char *lineBuffer = (char *)malloc(64);
	char *lineBufferCopy = (char *)malloc(64);
	createOutFile(file1Ticks);
	createOutFile(file2NSec);
	for (int i = 0; i < 64; i++)
	{
		lineBuffer[i] = '1';
	}
	printf("size1: %ld\tsize2: %ld\n", sizeof(lineBuffer), sizeof(lineBufferCopy));
	clock = 0;

	for (rep = 0; rep < REPEAT; rep++)
	{
		start = rdtsc();
		memcpy(lineBufferCopy, lineBuffer, 64);
		end = rdtsc();
		clflush(lineBuffer);
		clflush(lineBufferCopy);
		clock = clock + (end - start);
	}
	uint64_t averageTicksToBeCopied = 0, totalTimeForOps = 0;
	calcTimes(lineBufferCopy, lineBuffer, 64, &averageTicksToBeCopied, &totalTimeForOps);
	printf("%llu ticks to copy 64B\n", (long long unsigned int)(end - start));
	printf("took %llu ticks total\n", (long long unsigned int)clock);
	float timeInSec = (averageTicksToBeCopied / ticksPSec);
	printf("Time in sec:%f\n", timeInSec);
}

int main(int ac, char **av)
{
	long int numOfTicksPerSec = sysconf(_SC_CLK_TCK);
	printf("\nnumber of ticks in a second:%ld\n", numOfTicksPerSec);
	memtest(numOfTicksPerSec);
	return 0;
}
