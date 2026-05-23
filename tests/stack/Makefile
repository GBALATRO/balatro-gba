
CC := gcc
CFLAGS := -I../../include -I../../include/data_structures -I. \
          -g -O3 -std=gnu23 -Wall -Werror
SRC            := stack_test.c         \
                  ../../source/data_structures/stack.c
OUT            := build/stack_test 

$(OUT): $(SRC) | build
	$(CC) $(CFLAGS) -o $@ $^ 

build:
	mkdir -p build

clean:
	rm -f $(OUT)
