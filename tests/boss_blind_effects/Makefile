CC     := gcc
CFLAGS := -I../../include -I. \
          -g -O3 -std=gnu23 -Wall -Werror

SRC := boss_blind_effects_test.c ../../source/boss_blind_effects.c
OUT := build/boss_blind_effects_test

$(OUT): $(SRC) | build
	$(CC) $(CFLAGS) -o $@ $^

build:
	mkdir -p build

clean:
	rm -f $(OUT)
