#include "printk.hpp"

// Function to output a character to the console (platform-specific)
extern "C" void putchar(char c);

// Print a null-terminated string to the console
void printk(const char* str) {
    if (!str) return;  // Handle null pointer case
    
    while (*str) {
        putchar(*str++);
    }
}

extern "C" void putchar(char c) {
#if defined(__i386__) || defined(__x86_64__)  // Combined x86 and x86_64 case
    asm volatile (
        "movb $0x0E, %%ah\n\t"  // Set BIOS teletype output function
        "movb %[ch], %%al\n\t"  // Move character to AL
        "int $0x10"             // Call BIOS interrupt
        :                       // No outputs
        : [ch] "r"(c)           // Input operand
        : "ax"                  // Clobbers ax register (combines ah and al)
    );
#else
    #error "Unsupported architecture"
#endif
}