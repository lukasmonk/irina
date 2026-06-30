OBJS = build/main.o build/loop.o build/board.o build/data.o build/util.o \
       build/movegen.o build/makemove.o build/perft.o build/eval.o \
       build/evalst.o build/search.o build/person.o build/hash.o \
       build/tt.o build/book.o

CC = gcc
CFLAGS = -std=gnu17 -m64 -O2 -DNDEBUG
LDFLAGS = -m64 -static -static-libgcc

ifeq ($(OS),Windows_NT)
CFLAGS += -DWIN32
LINK_TARGET = irina.exe
else
LINK_TARGET = irina
endif

.PHONY: all clean

all: $(LINK_TARGET)

build/%.o: %.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

$(LINK_TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)
	strip $(LINK_TARGET)

clean:
	rm -f $(OBJS) $(LINK_TARGET)
	rm -rf build

build:
	mkdir build
