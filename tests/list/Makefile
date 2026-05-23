
CC := gcc
CFLAGS := -I../../include -I../../include/data_structures -I. \
          -g -O3 -std=gnu23 -Wall -Werror -DPOOLS_TEST_ENV=yes

SRC            := list_test.c          \
                  ../../source/data_structures/list.c  \
                  ../../source/data_structures/pool.c  \
                  ../../source/data_structures/bitset.c
OUT            := build/list_test 

$(OUT): $(SRC) | build
	$(CC) $(CFLAGS) -o $@ $^ 

build:
	mkdir -p build

clean:
	rm -f $(OUT)
