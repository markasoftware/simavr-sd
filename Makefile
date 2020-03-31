examples := ${wildcard examples/*.c}
examples := ${examples:.c=}
examples := examples/CardInfo

all: ${examples}

src/sd.o: src/sd.c
	${CC} ${CFLAGS} -c -o $@ -Wall -g $<

examples/%: examples/%.c src/sd.h src/sd.o
	${CC} ${CFLAGS} -o$@.out -Isrc -Wall -lsimavr -lelf -g src/sd.o $<

clean:
	rm src/sd.o examples/CardInfo.out

.PHONY: all clean
