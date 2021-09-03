BINS = user/test_proc user/test_thread
CFLAGS = -Wall -g

all: $(BINS)
	$(MAKE) -C module all

user/%: user/%.c
	$(CC) $(CFLAGS) -o $@ $< -lpthread

clean:
	$(RM) $(BINS)
	$(MAKE) -C module clean
