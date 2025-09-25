CFLAGS = -Wall -O0
CC = gcc
AUTOGENFILES = out.txt output.txt

TARGET_A = outA
INPUT_A = mainTempRefactored.c

TARGET_B = outB
INPUT_B = main.c

TARGET_C = outC
INPUT_C = MemTestWithMultipleSizes.c

all: $(TARGET_A) $(TARGET_B) $(TARGET_C)

$(TARGET_A): $(INPUT_A)
	@echo "Compiling $(INPUT_A)"
	$(CC) $(CFLAGS) -o $(TARGET_A) $(INPUT_A)

$(TARGET_B): $(INPUT_B)
	@echo "Compiling $(INPUT_B)"
	$(CC) $(CFLAGS) -o $(TARGET_B) $(INPUT_B)

$(TARGET_C): $(INPUT_C)
	@echo "Compiling $(INPUT_C)"
	$(CC) $(CFLAGS) -o $(TARGET_C) $(INPUT_C)

checkRDTSC: checkRDTSCP.c
	@echo "Compiling checkRDTSCP prog"
	$(CC) $(CFLAGS) -o checkRDTSCP checkRDTSCP.c
	@echo "Checking if RDTSCP is supported..."
	./checkRDTSCP

clean:
	@echo "Cleaning the build and output files"
	rm -f $(TARGET_A) $(TARGET_B) $(TARGET_C) $(AUTOGENFILES)

