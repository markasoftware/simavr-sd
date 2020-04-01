all: examples/simple

src/sd.o: src/sd.c
	${CC} ${CFLAGS} -c -o $@ -Wall -g $<

examples/%: examples/%.c src/sd.h src/sd.o
	${CC} ${CFLAGS} -o$@ -Isrc -Wall -lsimavr -lelf -g src/sd.o $<

clean:
	rm -f src/sd.o examples/simple

.PHONY: all clean
