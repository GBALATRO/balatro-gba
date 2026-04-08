GRIT   := /opt/devkitpro/tools/bin/grit
PREFIX := $(DEVKITPRO)/devkitARM/bin/arm-none-eabi-
BUILD  := build

IMAGES := $(wildcard graphics/*.png)
SFILES := $(patsubst graphics/%.png,$(BUILD)/%.s,$(IMAGES))
OFILES := $(patsubst graphics/%.png,$(BUILD)/%.o,$(IMAGES))

all: lib/libjoker_effects.a

lib/libjoker_effects.a: $(BUILD)/joker_effects.o $(OFILES)
	mkdir -p lib && $(PREFIX)ar rcs $@ $^

$(BUILD)/joker_effects.o: joker_effects.c $(SFILES) | $(BUILD)
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
