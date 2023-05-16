CC = gcc
CFLAGS = -Wall -Wextra

SOURCES := \
wrapper_functions.c \
file.c \
queue.c \
compress.c

TARGET = zipper

all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(TARGET).c $(SOURCES:.c=.o)
	$(CC) $(CFLAGS) -o $@ $< $(SOURCES:.c=.o) 

run: $(TARGET).exe
	./$(TARGET).exe

clean:
	rm -f $(SOURCES) $(TARGET)