#include "printk.hpp"
#include <stdint.h>

// Function to output a character to the console (platform-specific)
extern "C" void putchar(char c);

// Print a null-terminated string to the console
void printk(const char* str) {
    if (!str) return;  // Handle null pointer case
    
    while (*str) {
        putchar(*str++);
    }
}
// You can't use BIOS interrupts in 32-bit protected mode
#if 0
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
#endif

// Until you write a console driver to output to the display we will write
// strings using the Port E9 hack available in QEMU and BOCHS. This text
// will be displayed to the debug console.
//
// In bochsrc.txt add/edit the line:
//     port_e9_hack: enabled=1
//
// When running QEMU use the command line option:
//     -debugcon stdio
//
extern "C" void putchar(char c) {
    asm volatile (
        "outb %[ch], %[port]"             // Call BIOS interrupt
        :                       // No outputs
        : [ch] "a"(c), [port] "Nd" ((uint16_t)0xe9)           // Input operand
        : "memory"                  // Clobbers ax register (combines ah and al)
    );
}
