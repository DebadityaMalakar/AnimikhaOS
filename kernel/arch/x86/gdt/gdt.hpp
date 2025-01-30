#ifndef GDT_HPP
#define GDT_HPP

#include <stdint.h>

struct gdt_entry {
    uint32_t limit;
    uint32_t base;
    uint8_t access;
    uint8_t granularity;
};

struct tss_entry {
    uint32_t esp0;
    // Other TSS fields
};

// Initialize the GDT
void initialize_gdt();


#endif