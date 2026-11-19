
CC := gcc
CFLAGS := -I../../include -I. \
          -g -O3 -std=gnu23 -Wall -Werror
SRC            := run_history_test.c         \
                  ../../source/run_history.c
OUT            := build/run_history_test

$(OUT): $(SRC) | build
	$(CC) $(CFLAGS) -o $@ $^

build:
	mkdir -p build

clean:
	rm -f $(OUT)
