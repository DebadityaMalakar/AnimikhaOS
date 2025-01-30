#include <stdint.h>
#include "gdt.hpp"


// struct gdt_entry {
//     uint32_t limit;
//     uint32_t base;
//     uint8_t access;
//     uint8_t granularity;
// };

// struct tss_entry {
//     uint32_t esp0;
//     // Other TSS fields
// };

// Initialize the GDT
void initialize_gdt() {
    // Create GDT entries
    gdt_entry gdt[3];
    gdt[0].limit = 0;
    gdt[0].base = 0;
    gdt[0].access = 0;
    gdt[0].granularity = 0;

    gdt[1].limit = 0xFFFFF;
    gdt[1].base = 0;
    gdt[1].access = 0b10100;
    gdt[1].granularity = 0b11;

    gdt[2].limit = 0xFFFFFFFF;
    gdt[2].base = 0;
    gdt[2].access = 0b11100;
    gdt[2].granularity = 0b11;

    // Initialize TSS
    tss_entry tss;
    tss.esp0 = 0;
    // Initialize other TSS fields

    // Set up GDT descriptor
    uint32_t gdt_pointer = (uint32_t)&gdt;
    uint32_t tss_pointer = (uint32_t)&tss;

    // Flush GDT
    __asm__ __volatile__("lgdt %0" : : "m" (gdt_pointer));

    // Flush TSS
    __asm__ __volatile__("ltr %0" : : "r" (tss_pointer));
}
