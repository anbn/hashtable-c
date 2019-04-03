CC=cc
CFLAGS=-std=c99 -pedantic -Wall -ggdb
LIBS=
TARGET=ht

OBJECTS=$(patsubst %.c, %.o, $(wildcard *.c))
HEADERS=$(wildcard *.h)

.PRECIOUS: $(TARGET) $(OBJECTS)
.PHONY: default all clean

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

all: $(TARGET)

vg: all
	valgrind --leak-check=full ./$(TARGET)

clean:
	-rm -f *.o $(TARGET)
