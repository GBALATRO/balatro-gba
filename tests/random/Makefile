include $(DEVKITARM)/gba_rules
LIBTONC := $(DEVKITPRO)/libtonc

export INCLUDES := -I../../include -I. \
                   -I$(LIBGBA)/include \
		           -I$(LIBTONC)/include

export LIBPATHS	:= -L$(LIBGBA)/lib \
                   -L$(LIBTONC)/lib

CC := gcc
CFLAGS := $(INCLUDES) \
          -g -O3 -std=gnu23 -Wall -Werror -Wno-format

SRC            := random_test.c ../../source/random.c
OUT            := build/random_test

$(OUT): $(SRC) | build
	$(CC) $(CFLAGS) -o $@ $^ 

build:
	mkdir -p build

clean:
	rm -f $(OUT)
