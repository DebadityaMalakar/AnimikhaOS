#include "memory_management.hpp"
#include "../io/printk.hpp"


extern "C" void putchar(char c);

namespace Memory {
    void MemoryManager::initialize() {
        // Memory detection is already done by main.asm
        parseMemoryMap();
        printMemoryInfo();
    }

    MemoryMapEntry* MemoryManager::getMemoryMap() {
        return memory_map;
    }

    uint32_t MemoryManager::getEntryCount() {
        return entries_counter;
    }

    uint64_t MemoryManager::getTotalRAM() {
        uint64_t total = 0;
        MemoryMapEntry* entries = getMemoryMap();
        uint32_t count = getEntryCount();

        for (uint32_t i = 0; i < count; i++) {
            total += entries[i].length;
        }
        return total;
    }

    uint64_t MemoryManager::getAvailableRAM() {
        uint64_t available = 0;
        MemoryMapEntry* entries = getMemoryMap();
        uint32_t count = getEntryCount();

        for (uint32_t i = 0; i < count; i++) {
            if (entries[i].type == static_cast<uint32_t>(MemoryType::FREE)) {
                available += entries[i].length;
            }
        }
        return available;
    }

    void MemoryManager::parseMemoryMap() {
        // Additional parsing can be added here if needed
    }

    void MemoryManager::printMemoryInfo() {
        uint64_t totalRAM = getTotalRAM();
        uint64_t availableRAM = getAvailableRAM();
        uint32_t entryCount = getEntryCount();

        // Convert to MB for display
        uint64_t totalMB = totalRAM / (1024 * 1024);
        uint64_t availableMB = availableRAM / (1024 * 1024);

        printk("Memory Information:\n");
        printk("Total RAM: ");
        // Convert number to string and print
        char buffer[32];
        int pos = 0;
        uint64_t num = totalMB;
        do {
            buffer[pos++] = '0' + (num % 10);
            num /= 10;
        } while (num > 0);
        while (pos > 0) {
            putchar(buffer[--pos]);
        }
        printk(" MB\n");

        printk("Available RAM: ");
        pos = 0;
        num = availableMB;
        do {
            buffer[pos++] = '0' + (num % 10);
            num /= 10;
        } while (num > 0);
        while (pos > 0) {
            putchar(buffer[--pos]);
        }
        printk(" MB\n");

        printk("Memory Map Entries: ");
        pos = 0;
        num = entryCount;
        do {
            buffer[pos++] = '0' + (num % 10);
            num /= 10;
        } while (num > 0);
        while (pos > 0) {
            putchar(buffer[--pos]);
        }
        printk("\n");

        // Print each memory region
        MemoryMapEntry* entries = getMemoryMap();
        for (uint32_t i = 0; i < entryCount; i++) {
            printk("Region ");
            pos = 0;
            num = i;
            do {
                buffer[pos++] = '0' + (num % 10);
                num /= 10;
            } while (num > 0);
            while (pos > 0) {
                putchar(buffer[--pos]);
            }
            printk(": Type=");

            switch (entries[i].type) {
                case static_cast<uint32_t>(MemoryType::FREE):
                    printk("Available");
                    break;
                case static_cast<uint32_t>(MemoryType::RESERVED):
                    printk("Reserved");
                    break;
                case static_cast<uint32_t>(MemoryType::ACPI):
                    printk("ACPI");
                    break;
                case static_cast<uint32_t>(MemoryType::ACPI_NVS):
                    printk("ACPI NVS");
                    break;
                case static_cast<uint32_t>(MemoryType::BAD):
                    printk("Bad Memory");
                    break;
                default:
                    printk("Unknown");
            }
            printk("\n");
        }
    }
}
