
CC := gcc
CFLAGS := -I../../include -I. -I../../include/data_structures \
          -g -O3 -std=gnu23 -Wall -Werror
SRC            := bitset_test.c         \
                  ../../source/data_structures/bitset.c
OUT            := build/bitset_test 

$(OUT): $(SRC) | build
	$(CC) $(CFLAGS) -o $@ $^ 

build:
	mkdir -p build

clean:
	rm -f $(OUT)
