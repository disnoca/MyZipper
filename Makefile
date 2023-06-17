CC = gcc
CFLAGS = -Wall -Wextra

SOURCES := \
src/wrapper_functions.c \
src/queue.c \
src/concurrency.c \
src/zipper/zipper_file.c \
src/zipper/crc32.c \
src/zipper/compression/no_compression/no_compression.c \
src/zipper/zipper.c

TARGET = zipper.exe

all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(SOURCES:.c=.o)
	$(CC) $(CFLAGS) -o $@ $(SOURCES:.c=.o)

clean:
	del /q *.o $(TARGET)