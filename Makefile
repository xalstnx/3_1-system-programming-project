CC=gcc
CFLAGS=-o
TARGETS=2016003736
all: $(TARGETS)
.PHONY: all
%:
	$(CC) -g -o $@ $@.c
       
clean:
	rm $(TARGETS)
