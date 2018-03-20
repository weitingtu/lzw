CC = g++
CFLAG = -O2 -lpthread
ifeq ($(MODE),rd)
	CFLAG:=$(filter-out -O2, $(CFLAG))
    CFLAG += -g3 -gdwarf-4 -ggdb3 -DDEBUG
endif

all: lzw new_lzw.o

new_lzw.o : new_lzw.c 
	$(CC) -o $@ -c $< $(CFLAG)

lzw: new_lzw.o 
	$(CC) -o $@ $^ $(CFLAG)

clean:
	rm -f lzw new_lzw.o 
