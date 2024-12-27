# ./Makefile

SUBDIRS = bootloader
BUILD_DIR = build

.PHONY: all clean

all: $(BUILD_DIR)
	$(MAKE) -C bootloader

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run-bootloader: 
	qemu-system-i386 -fda build/bootloader.bin

clean:
	@for dir in $(SUBDIRS); do \
	    $(MAKE) -C $$dir clean; \
	done
	rm -rf $(BUILD_DIR)
