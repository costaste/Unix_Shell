
BIN  := nush
SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)

CFLAGS := -g
LDLIBS :=

$(BIN): $(OBJS)
	$(CC) -std=c99 -o $@ $(OBJS) $(LDLIBS)

%.o : %.c $(wildcard *.h)
	$(CC) -std=c99 $(CFLAGS) -c -o $@ $<

clean:
	rm -rf *.o $(BIN) tmp *.plist valgrind.out main.out

test: $(BIN)
	perl test.pl

valgrind: $(BIN)
	valgrind -q -v --leak-check=full --log-file=valgrind.out ./$(BIN)

.PHONY: clean test
