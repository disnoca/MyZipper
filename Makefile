CC = gcc
CFLAGS = -Wall -Wextra

SOURCES := \
wrapper_functions.c \
file.c \
queue.c \
crc32.c \
compression/no_compression/no_compression.c

TARGET = zipper

all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(TARGET).c $(SOURCES:.c=.o)
	$(CC) $(CFLAGS) -o $@ $< $(SOURCES:.c=.o) 

run: $(TARGET).exe
	./$(TARGET).exe

clean:
	rm -fi $(SOURCES.c=.o) $(TARGET).exe