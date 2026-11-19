
CC := gcc
CFLAGS := -I../../include -I. \
          -g -O3 -std=gnu23 -Wall -Werror
SRC            := sfx_pitch_test.c         \
                  ../../source/sfx_pitch.c
OUT            := build/sfx_pitch_test

$(OUT): $(SRC) | build
	$(CC) $(CFLAGS) -o $@ $^

build:
	mkdir -p build

clean:
	rm -f $(OUT)
