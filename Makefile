INCLUDE_PATH = ./lib

CFLAGS   = -Wall -g -O3

OBJS = coro.o \
		socket.o

TEST_OBJS = coro \
			socket

$(TEST_OBJS): $(OBJS)
	$(CC) -o $@ $^ tests/$@.c -I $(INCLUDE_PATH) $(CFLAGS)

%.o: lib/%.c
	$(CC) -o $@ -c $< $(CFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< -I $(INCLUDE_PATH) $(CFLAGS)

%.S: lib/%.c
	$(CC) -o $@ -S $< $(CFLAGS)

%.S: %.c
	$(CC) -o $@ -S $<  -I $(INCLUDE_PATH) $(CFLAGS)

clean:
	-@rm $(TEST_OBJS) *.o *.S
