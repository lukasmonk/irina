ifeq ($(OS),Windows_NT)
LINK_TARGET = irina.exe
RM = del /Q
else
LINK_TARGET = irina
RM = rm -f
endif

OBJS = main.o loop.o board.o data.o util.o movegen.o makemove.o perft.o eval.o evalst.o search.o person.o hash.o book.o

REBUILDABLES = $(OBJS) $(LINK_TARGET)

clean:
	$(RM) $(REBUILDABLES)
	echo Clean done

all: $(LINK_TARGET)
	echo All done

$(LINK_TARGET): $(OBJS)
	gcc -O3 -o $@ $^ -DNDEBUG
	strip $(LINK_TARGET)

%.o: %.c
	gcc -Wall -O3 -o $@ -c $< -DNDEBUG
