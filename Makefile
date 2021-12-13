INCLUDE_PATH = ./lib

CFLAGS   = -Wall -g -O3

test: test.o coro.o
	$(CC) -o $@ $^ $(CFLAGS)

test_asm: test.S coro.S

%.o: lib/%.c
	$(CC) -o $@ -c $< $(CFLAGS)

%.o: %.c
	$(CC) -I $(INCLUDE_PATH) -o $@ -c $< $(CFLAGS)

%.S: lib/%.c
	$(CC) -o $@ -S $< $(CFLAGS)

%.S: %.c
	$(CC) -I $(INCLUDE_PATH) -o $@ -S $< $(CFLAGS)

clean:
	-@rm test *.o *.S
