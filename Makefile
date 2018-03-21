CC = g++
CFLAG = -O2 -lpthread -ansi
ifeq ($(MODE),rd)
	CFLAG:=$(filter-out -O2, $(CFLAG))
    CFLAG += -g3 -gdwarf-4 -ggdb3 -DDEBUG
endif

TEST_DIR = test_dir

all: lzw new_lzw.o

new_lzw.o : new_lzw.c 
	$(CC) -o $@ -c $< $(CFLAG)

lzw: new_lzw.o 
	$(CC) -o $@ $^ $(CFLAG)

clean:
	rm -rf lzw new_lzw.o $(TEST_DIR)

test: all
	rm -rf $(TEST_DIR); \
	mkdir $(TEST_DIR); \
	cd $(TEST_DIR); \
	for size in 100k 1M 10M; do \
	head -c $$size < /dev/urandom > $$size.img; \
	cp $$size.img $$size.img.orig; \
	md5sum $$size.img > md5; \
	../lzw -c compress $$size.img; \
	../lzw -d compress; \
	md5sum -c md5; \
	done; \
	md5sum *.img > md5; \
	../lzw -c compress *.img; \
	../lzw -d compress; \
	md5sum -c md5

