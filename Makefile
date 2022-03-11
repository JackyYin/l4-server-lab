INCLUDE_PATH = ./include

CFLAGS   = -Wall -g -O3 -pthread
DEFINES  = -D_GNU_SOURCE

OBJS = coro.o \
		socket.o \
		server.o

TEST_OBJS = coro \
			hashtable \
			socket

$(TEST_OBJS): $(OBJS)
	$(CC) -o $@ $^ tests/$@.c -I $(INCLUDE_PATH) $(CFLAGS)

%.o: lib/%.c
	$(CC) -o $@ -c $<  -I $(INCLUDE_PATH) $(CFLAGS) $(DEFINES)

%.o: %.c
	$(CC) -o $@ -c $< -I $(INCLUDE_PATH) $(CFLAGS) $(DEFINES)

%.S: lib/%.c
	$(CC) -o $@ -S $< $(CFLAGS) $(DEFINES)

%.S: %.c
	$(CC) -o $@ -S $<  -I $(INCLUDE_PATH) $(CFLAGS) $(DEFINES)

clean:
	-@rm $(TEST_OBJS) *.o *.S
