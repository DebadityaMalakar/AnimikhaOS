bits 16
section .text.startup
global start

start:
    ; Note: we are not in 32-bit protected mode at this point
    ; We are still in 16-bit real mode. Eventually when you
    ; call into 32-bit C code you will have to enter 32-bit
    ; protected mode first.
    cli                  ; Clear interrupts
    cld                  ; Clear direction flag
    xor ax, ax           ; Zero out AX
    mov ds, ax           ; Set DS to 0x0000
    mov es, ax           ; Set ES to 0x0000
                         ; Real mode stack set in bootloader

    ; Call the memory detection function
    call detect_memory

    ; Infinite loop to halt the CPU
hang:
    hlt                  ; Halt the CPU
    jmp hang             ; Infinite loop

; Memory detection function using INT 15h, E820h
detect_memory:
    pushad                      ; Save all registers
    xor ebx, ebx                ; ebx must be 0 to start
    mov edx, 0x534D4150         ; Place "SMAP" into edx
    mov edi, memory_map         ; Set EDI to the start of our memory_map buffer

.repeat:
    mov eax, 0xE820             ; eax = 0xE820
    mov ecx, 24                 ; Ask for 24 bytes
    xor esi, esi                ; Zero out ESI
    int 0x15                    ; Call BIOS
    jc .failed                  ; Carry set means "end of list already reached"
    cmp eax, 0x534D4150         ; On success, eax must have been reset to "SMAP"
    jne .failed

    ; We got a valid entry, store size of entries counter at entries_counter
    ; and increment it
    inc dword [entries_counter]

    ; Go to next entry
    add edi, 24                 ; Move to the next entry
    test ebx, ebx               ; If ebx = 0, we're done
    jnz .repeat                 ; Get next entry

.done:
    popad                       ; Restore all registers
    ret

.failed:
    mov dword [entries_counter], 0  ; If failed, store 0 as number of entries
    jmp .done

section .bss
memory_map resb 256 * 24        ; Reserve space for 256 entries, each 24 bytes
entries_counter resd 1          ; Reserve space for the number of entries
