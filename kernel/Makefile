# Compiler settings
NASM = nasm
CXX = g++
LD = ld
OBJCOPY = objcopy

# Flags
NASMFLAGS = -f elf32
CXXFLAGS = -m32 -fno-pie -fno-stack-protector -nostdlib -fno-rtti -fno-exceptions -ffreestanding -O2
LDFLAGS = -m elf_i386 -T kernel.ld

# Directories
KERNEL_DIR = .
IO_DIR = $(KERNEL_DIR)/io
MEMORY_DIR = $(KERNEL_DIR)/memory_management
BUILD_DIR = ../build/kernel

# Source files
ASM_SOURCES = $(KERNEL_DIR)/main/main.asm
CPP_SOURCES = $(IO_DIR)/printk.cpp \
              $(MEMORY_DIR)/memory_management.cpp

# Object files
ASM_OBJECTS = $(patsubst $(KERNEL_DIR)/%.asm,$(BUILD_DIR)/%.o,$(ASM_SOURCES))
CPP_OBJECTS = $(patsubst $(KERNEL_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CPP_SOURCES))
ALL_OBJECTS = $(ASM_OBJECTS) $(CPP_OBJECTS)

# Output kernel binary
KERNEL = $(BUILD_DIR)/kernel.bin
KERNEL_ELF = $(BUILD_DIR)/kernel.elf

# Make sure these directories exist
NEEDED_DIRS = $(dir $(ASM_OBJECTS) $(CPP_OBJECTS))

# Default target
all: directories $(KERNEL)

# Create necessary directories
directories:
	mkdir -p $(NEEDED_DIRS)

# Compile assembly files
$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.asm
	$(NASM) $(NASMFLAGS) $< -o $@

# Compile C++ files
$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Link everything together to an ELF file
$(KERNEL_ELF): $(ALL_OBJECTS)
	$(LD) $(LDFLAGS) $(ALL_OBJECTS) -o $@

# Convert kernel elf file to bin file
$(KERNEL): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@

# Clean build files
clean:
	rm -rf $(BUILD_DIR)/*

.PHONY: all clean directories
