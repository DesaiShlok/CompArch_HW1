CFLAGS = -Wall -O0
CC = gcc

TARGET = MemTest
INPUT = MemTest.c


all: $(TARGET)

$(TARGET): $(INPUT)
	@echo "Compiling $(INPUT)"
	$(CC) $(CFLAGS) -D_GNU_SOURCE -o $(TARGET) $(INPUT)

runC:
	sudo cpupower frequency-set -g userspace #switch to userspace governer
	sudo cpupower frequency-set -d 3000MHz -u 3000MHz #set min max freq locks
	echo 0 | sudo tee /sys/devices/system/cpu/cpufreq/boost
	sudo cpupower idle-set -D 0
	taskset -c 2 ./$(TARGET) #pins the c file execution to core 2 and runs it

clean:
	@echo "Cleaning the build and output files"
	rm -f $(TARGET) *.txt

