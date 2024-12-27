; bootloader.asm
[org 0x7C00]        ; Origin address for the bootloader
bits 16             ; Real mode (16-bit)

start:
    cli             ; Clear interrupts
    xor ax, ax      ; Zero out AX
    mov ds, ax      ; Set Data Segment to 0x0000
    mov es, ax      ; Set Extra Segment to 0x0000

    ; Print "Hello World!" to the screen
    mov si, message
print_loop:
    lodsb           ; Load byte at [SI] into AL and increment SI
    or al, al       ; Check if AL is 0 (end of string)
    jz done         ; If zero, jump to done
    mov ah, 0x0E    ; BIOS teletype function
    int 0x10        ; Call BIOS interrupt to print character
    jmp print_loop  ; Loop back for next character

done:
    hlt             ; Halt the CPU
    jmp $           ; Infinite loop to prevent execution from continuing

message db 'AnimikhaOS is booting', 0  ; Null-terminated string

; Bootloader signature (magic number for bootable media)
times 510 - ($ - $$) db 0     ; Fill up to 510 bytes
dw 0xAA55                     ; Bootloader signature
