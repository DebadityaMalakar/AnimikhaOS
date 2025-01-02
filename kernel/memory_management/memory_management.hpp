#ifndef MEMORY_MANAGEMENT_HPP
#define MEMORY_MANAGEMENT_HPP

#include <stdint.h>


namespace Memory {
    // Structure to hold memory map entry information
    struct MemoryMapEntry {
        uint64_t base;
        uint64_t length;
        uint32_t type;
        uint32_t acpi;
    } __attribute__((packed));

    // Memory types as defined by BIOS INT 15H, E820H
    enum class MemoryType {
        FREE        = 1,
        RESERVED   = 2,
        ACPI       = 3,
        ACPI_NVS   = 4,
        BAD        = 5
    };

    class MemoryManager {
    public:
        static void initialize();
        static uint64_t getTotalRAM();
        static uint64_t getAvailableRAM();
        static void printMemoryInfo();

    private:
        static void parseMemoryMap();
        static MemoryMapEntry* getMemoryMap();
        static uint32_t getEntryCount();
    };

    // These are exported from main.asm
    extern "C" MemoryMapEntry memory_map[];
    extern "C" uint32_t entries_counter;
}

#endif // MEMORY_MANAGEMENT_HPP
