CC = gcc
CFLAGS = -Wall -Wextra

SOURCES := \
wrapper_functions.c \
zipper_file.c \
queue.c \
crc32.c \
concurrency.c \
compression/no_compression/no_compression.c

TARGET = zipper

all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(TARGET).c $(SOURCES:.c=.o)
	$(CC) $(CFLAGS) -o $@ $< $(SOURCES:.c=.o) 

clean:
	del /q *.o $(TARGET).exe