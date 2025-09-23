CFLAGS = -Wall -O0
TARGET = out
AUTOGENFILES = out.txt output.txt
INPUT = main.c
all: $(TARGET)
CC = gcc
$(TARGET): $(INPUT)
	$(CC) $(CFLAGS) -o $(TARGET) $(INPUT)
clean:
	rm -f $(TARGET) $(AUTOGENFILES)
