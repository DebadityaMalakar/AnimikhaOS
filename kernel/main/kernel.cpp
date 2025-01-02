#include "../memory_management/memory_management.hpp"

volatile Memory::MemoryManager mm;

extern "C" void kmain(void)
{
    mm.initialize();
    return;
}

