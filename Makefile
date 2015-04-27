CC := gcc
CFLAGS := -Wall -lxcb

tartwm : tartwm.c
	$(CC) $(CFLAGS) -o $@ $<
