#ifndef GDT_HPP
#define GDT_HPP

#include <stdint.h>

// Define segment descriptor struct
struct gdt_entry_struct {
    uint16_t limit_low;     // Lower 16 bits of the limit
    uint16_t base_low;      // Lower 16 bits of the base
    uint8_t base_middle;    // Next 8 bits of the base
    uint8_t access;         // Access flags
    uint8_t granularity;    // Granularity and high 4 bits of limit
    uint8_t base_high;      // Last 8 bits of the base
} __attribute__((packed));

typedef struct gdt_entry_struct gdt_entry_t;

// Define GDT pointer struct
struct gdt_ptr_struct {
    uint16_t limit;   // Size of GDT - 1
    uint32_t base;    // Base address of GDT
} __attribute__((packed));

typedef struct gdt_ptr_struct gdt_ptr_t;

// Define TSS structure
struct tss_entry {
    uint32_t prev_tss;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t unused[23]; // Other TSS fields
} __attribute__((packed));

typedef struct tss_entry tss_entry_t;

// Function to initialize GDT
void initialize_gdt();

#endif
