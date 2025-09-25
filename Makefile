CFLAGS = -Wall -O0
TARGET = outTempRef
AUTOGENFILES = out.txt output.txt
INPUT = mainTempRefactored.c
all: $(TARGET)
CC = gcc
$(TARGET): $(INPUT)
	@echo "Compiling main.c\n\n"
	$(CC) $(CFLAGS) -o $(TARGET) $(INPUT)
checkRDTSC: checkRDTSCP.c
	@echo "Compiling checkRDTSCP prog\n\n"
	$(CC) $(CFLAGS) -o checkRDTSCP checkRDTSCP.c
	@echo "Checking if RDTSCP is supported for this processor\n.\n."
	./checkRDTSCP

clean:
	@echo "Cleaning the build and output files\n"
	rm -f $(TARGET) $(AUTOGENFILES)
