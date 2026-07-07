NT_API_PATH := distingNT_API
INCLUDE_PATH := $(NT_API_PATH)/include
SRC := vortex.cpp
OUTPUT := plugins/vortex.o
MANIFEST := plugins/plugin.json
VERSION := $(shell cat VERSION)

# Prefer official ARM toolchain (includes C++ stdlib), fall back to Homebrew's bare-metal GCC
ARM_TC := $(HOME)/arm-gnu-toolchain/arm-gnu-toolchain-15.2.rel1-darwin-arm64-arm-none-eabi/bin
ifeq ($(wildcard $(ARM_TC)/arm-none-eabi-c++),)
CC := arm-none-eabi-c++
else
CC := $(ARM_TC)/arm-none-eabi-c++
endif
CFLAGS := -std=c++11 -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard \
          -mthumb -fno-rtti -fno-exceptions -Os -fPIC -Wall \
          -I$(INCLUDE_PATH) \
          -DVORTEX_VERSION='"$(VERSION)"'

all: $(OUTPUT) $(MANIFEST)

$(OUTPUT): $(SRC) dsp.h VERSION
	mkdir -p plugins
	$(CC) $(CFLAGS) -c -o $@ $<

$(MANIFEST): VERSION
	mkdir -p plugins
	@echo '{"name": "Vortex", "guid": "Vrtx", "version": "$(VERSION)", "author": "wintoid", "description": "Vortex - multi-mode filter for Disting NT", "tags": ["effect", "filter"]}' > $@

clean:
	rm -f $(OUTPUT) $(MANIFEST)

.PHONY: all clean
