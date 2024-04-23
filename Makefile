CC ?= cc

SRC=$(wildcard src/*.c)
OBJ=$(SRC:.c=.o)

CFLAGS?=
CFLAGS+=-Isrc

LDFLAGS?=-Isrc

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
