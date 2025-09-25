CFLAGS = -Wall -O0
CC = gcc
AUTOGENFILES = out.txt output.txt

# Targets and sources
TARGET_A = outA
INPUT_A = mainTempRefactored.c

TARGET_B = outB
INPUT_B = main.c

TARGET_C = outC
INPUT_C = MemTestWithMultipleSizes.c

TARGET_RDTSC = checkRDTSCP
INPUT_RDTSC = checkRDTSCP.c

# Default build
all: $(TARGET_A) $(TARGET_B) $(TARGET_C)

# Compile targets
$(TARGET_A): $(INPUT_A)
	@echo "Compiling $(INPUT_A)"
	$(CC) $(CFLAGS) -o $(TARGET_A) $(INPUT_A)

$(TARGET_B): $(INPUT_B)
	@echo "Compiling $(INPUT_B)"
	$(CC) $(CFLAGS) -o $(TARGET_B) $(INPUT_B)

$(TARGET_C): $(INPUT_C)
	@echo "Compiling $(INPUT_C)"
	$(CC) $(CFLAGS) -o $(TARGET_C) $(INPUT_C)

# Compile and run checkRDTSCP
$(TARGET_RDTSC): $(INPUT_RDTSC)
	@echo "Compiling $(INPUT_RDTSC)"
	$(CC) $(CFLAGS) -o $(TARGET_RDTSC) $(INPUT_RDTSC)
	@echo "Checking if RDTSCP is supported..."
	./$(TARGET_RDTSC)

# Clean build
clean:
	@echo "Cleaning the build and output files"
	rm -f $(TARGET_A) $(TARGET_B) $(TARGET_C) $(TARGET_RDTSC) $(AUTOGENFILES) *.txt

