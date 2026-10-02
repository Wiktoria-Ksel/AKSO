.PHONY: all clean

CC = gcc
CFLAGS = -Wall -Wextra -Wno-implicit-fallthrough -std=gnu23 -fPIC -O2
LDFLAGS = -shared -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc \
          -Wl,--wrap=reallocarray -Wl,--wrap=free -Wl,--wrap=strdup -Wl,--wrap=strndup

librstack.so: rstack.o memory_tests.o
	$(CC) $(LDFLAGS) -o $@ $^

rstack.o: rstack.c rstack.h
	$(CC) $(CFLAGS) -c -o $@ $<

memory_tests.o: memory_tests.c memory_tests.h
	$(CC) $(CFLAGS) -c -o $@ $<

example: rstack_example.c librstack.so
	$(CC) $(CFLAGS) -o $@ $< -L. -lrstack -Wl,-rpath,.

tester: tests/test*.c librstack.so
	$(CC) --std=gnu23 tests/test*.c -L. -lrstack -o $@

clean:
	rm -f *.o *.so example tester
