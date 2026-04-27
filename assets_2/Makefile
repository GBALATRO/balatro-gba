GRIT   := /opt/devkitpro/tools/bin/grit
PREFIX := $(DEVKITPRO)/devkitARM/bin/arm-none-eabi-
BUILD  := build

IMAGES := $(wildcard graphics/*.png)
SFILES := $(patsubst graphics/%.png,$(BUILD)/%.s,$(IMAGES))
OFILES := $(patsubst graphics/%.png,$(BUILD)/%.o,$(IMAGES))

all: lib/libcool_mod.a

lib/libcool_mod.a: $(BUILD)/cool_mod.o $(OFILES)
	mkdir -p lib && $(PREFIX)ar rcs $@ $^

$(BUILD)/cool_mod.o: cool_mod.c $(SFILES) | $(BUILD)
	$(PREFIX)gcc -c $< -o $@ -I$(DEVKITPRO)/libgba/include -I$(DEVKITPRO)/libtonc/include -I../include -I../include -I./include -I./build

$(BUILD)/%.o: $(BUILD)/%.s
	$(PREFIX)as $< -o $@


$(BUILD)/%.s $(BUILD)/%.h: graphics/%.png graphics/%.grit | $(BUILD)
	$(GRIT) $< -o$(BUILD)/$*

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD) lib

.PHONY: all clean
