# bootloader/Makefile

TARGET = bootloader.bin
BUILD_DIR = ../build
ASM_SRC = bootloader.asm

NASM = nasm
NASM_FLAGS = -f bin

all: $(BUILD_DIR) $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/$(TARGET): $(ASM_SRC)
	$(NASM) $(NASM_FLAGS) $< -o $@

clean:
	rm -f $(BUILD_DIR)/$(TARGET)
