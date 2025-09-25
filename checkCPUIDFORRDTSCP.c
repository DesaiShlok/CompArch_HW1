#include <cpuid.h>
#include <stdio.h>
int rdtscp_supported(void) {
    unsigned a, b, c, d;
    if (__get_cpuid(0x80000001, &a, &b, &c, &d) && (d & (1<<27)))
    {
        // RDTSCP is supported.
	printf("CPUID: RDTSC is supported\n");
        return 1;
    }
    else{
    printf("CPUID: RDTSC is not supported\n");
    }
}

int main()
{
	rdtscp_supported();
	return 0;
}
