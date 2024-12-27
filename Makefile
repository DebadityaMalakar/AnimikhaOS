# ./Makefile

# SUBDIRS = bootloader
# BUILD_DIR = build

# .PHONY: all clean

# all: $(BUILD_DIR)
# 	$(MAKE) -C bootloader

# $(BUILD_DIR):
# 	mkdir -p $(BUILD_DIR)

# run-bootloader: 
# 	qemu-system-i386 -fda build/bootloader.bin

# clean:
# 	@for dir in $(SUBDIRS); do \
# 	    $(MAKE) -C $$dir clean; \
# 	done
# 	rm -rf $(BUILD_DIR)

# Directories
BOOTLOADER_DIR = bootloader
KERNEL_DIR = kernel
BUILD_DIR = build
KERNEL_BUILD_DIR = $(BUILD_DIR)/kernel

# Output files
DISK_IMG = $(BUILD_DIR)/disk.img
BOOTLOADER_BIN = $(BUILD_DIR)/bootloader.bin
KERNEL_BIN = $(KERNEL_BUILD_DIR)/kernel.bin

# Default target
all: $(DISK_IMG)

# Build bootloader and kernel creating a 1.44MiB floppy
$(DISK_IMG): $(BOOTLOADER_BIN) $(KERNEL_BIN)
	cat $(BOOTLOADER_BIN) $(KERNEL_BIN) > $(DISK_IMG)
	truncate -s 1440K $(DISK_IMG)

# Ensure bootloader is built
$(BOOTLOADER_BIN):
	$(MAKE) -C $(BOOTLOADER_DIR) all

# Ensure kernel is built
$(KERNEL_BIN):
	$(MAKE) -C $(KERNEL_DIR) all

run-bootloader: $(DISK_IMG)
	qemu-system-i386 -fda $<

# Clean build files
clean:
	$(MAKE) -C $(BOOTLOADER_DIR) clean
	$(MAKE) -C $(KERNEL_DIR) clean
	rm -f $(DISK_IMG)

.PHONY: all clean
