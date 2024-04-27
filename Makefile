CC ?= cc

SRC=$(wildcard src/*.c)
INCLUDES?=

override CFLAGS?=
override CFLAGS+=-Wall -Isrc
override LDFLAGS?=-Wall -Isrc

OBJ=$(SRC:.c=.o)

include lib/.dep/config.mk

override CFLAGS+=$(INCLUDES)

BIN=tinydns

default: $(BIN)

.c.o:
	$(CC) $(@:.o=.c) $(CFLAGS) -c -o $@

$(BIN): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) -o $@

.PHONY: clean
clean:
	rm -f $(OBJ)

.PHONY: sterile
sterile: clean
	rm -f $(BIN)
