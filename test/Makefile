
CC = gcc

CFLAGS = -I/usr/include/sai
LDFLAGS = -L/usr/lib
LDLIBS = -lsai

SRC = test.c
TARGET = bin/unit_test

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) -o $(TARGET) $(CFLAGS) $(LDFLAGS) $(SRC) $(LDLIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean