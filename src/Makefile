ifeq ($(OS),Windows_NT)
LINK_TARGET = irina.exe
DWIN= -DWIN32
else
LINK_TARGET = irina
DWIN=
endif

RM = rm -f

OBJS = build/main.o build/loop.o build/board.o build/data.o build/util.o build/movegen.o build/makemove.o build/perft.o build/eval.o build/evalst.o build/search.o build/person.o build/hash.o build/tt.o build/book.o

REBUILDABLES = $(OBJS) $(LINK_TARGET)

MKDIR = mkdir -p build
RMDIR = rm -rf build

clean:
	$(RM) $(REBUILDABLES)
	$(RMDIR)
	echo Clean done

all: $(LINK_TARGET)
	echo All done

$(LINK_TARGET): $(OBJS)
	gcc -flto -O3 -o $@ $^ -DNDEBUG
	strip $(LINK_TARGET)

build/%.o: %.c | build
	gcc -Wall -flto -O3 -o $@ -c $< -DNDEBUG $(DWIN)

build:
	$(MKDIR)
