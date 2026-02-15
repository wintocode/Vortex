NT_API_PATH := distingNT_API
INCLUDE_PATH := $(NT_API_PATH)/include
SRC := vortex.cpp
OUTPUT := plugins/vortex.o
MANIFEST := plugins/plugin.json
VERSION := $(shell cat VERSION)

CC := arm-none-eabi-c++
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
