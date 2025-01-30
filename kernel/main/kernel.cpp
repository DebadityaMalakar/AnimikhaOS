#include "../memory_management/memory_management.hpp"
#include "../arch/x86/gdt/gdt.hpp"

Memory::MemoryManager mm;

extern "C" void kmain(void)
{
    mm.initialize();
    initialize_gdt();
    return;
}

