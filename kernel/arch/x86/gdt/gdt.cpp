#include "gdt.hpp"

gdt_entry_t gdt[3];    // GDT with 3 entries
gdt_ptr_t gdt_ptr;     // GDT descriptor
tss_entry_t tss;       // Task State Segment

// Function to set a GDT entry
void set_gdt_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity) {
    gdt[index].base_low = base & 0xFFFF;
    gdt[index].base_middle = (base >> 16) & 0xFF;
    gdt[index].base_high = (base >> 24) & 0xFF;

    gdt[index].limit_low = limit & 0xFFFF;
    gdt[index].granularity = (limit >> 16) & 0x0F;

    gdt[index].granularity |= granularity & 0xF0;
    gdt[index].access = access;
}

// Function to initialize the GDT
void initialize_gdt() {
    // Set up the GDT pointer
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 3) - 1;
    gdt_ptr.base = (uint32_t)&gdt;

    // Null segment
    set_gdt_entry(0, 0, 0, 0, 0);

    // Code segment
    set_gdt_entry(1, 0, 0xFFFFF, 0x9A, 0xCF);

    // Data segment
    set_gdt_entry(2, 0, 0xFFFFF, 0x92, 0xCF);

    // Initialize TSS
    tss.esp0 = 0;
    tss.ss0 = 0x10;  // Data segment selector
    for (int i = 0; i < 23; i++) tss.unused[i] = 0;

    // Load GDT
    __asm__ __volatile__("lgdt %0" : : "m"(gdt_ptr));

    // Load segment registers
    __asm__ __volatile__(
        "mov $0x10, %ax\n"
        "mov %ax, %ds\n"
        "mov %ax, %es\n"
        "mov %ax, %fs\n"
        "mov %ax, %gs\n"
        "mov %ax, %ss\n"
    );

    // Far jump to reload CS
    __asm__ __volatile__(
        "ljmp $0x08, $.flush\n"
        ".flush:\n"
    );

    // Load TSS
    uint16_t tss_selector = 0x18; // TSS selector (entry index 3 in GDT)
    __asm__ __volatile__("ltr %0" : : "r"(tss_selector));
}
