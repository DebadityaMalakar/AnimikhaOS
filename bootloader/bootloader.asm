[org 0x7C00]        ; Origin address for the bootloader
bits 16             ; Real mode (16-bit)

start:
    cli             ; Clear interrupts
    xor ax, ax      ; Zero out AX
    mov ds, ax      ; Set Data Segment to 0x0000
    mov es, ax      ; Set Extra Segment to 0x0000

    ; Print "AnimikhaOS is booting" to the screen
    mov si, message
print_loop:
    lodsb           ; Load byte at [SI] into AL and increment SI
    or al, al       ; Check if AL is 0 (end of string)
    jz delay        ; If zero, jump to delay
    mov ah, 0x0E    ; BIOS teletype function
    int 0x10        ; Call BIOS interrupt to print character
    jmp print_loop  ; Loop back for next character

delay:
    ; Implement a 3-second delay using a busy-wait loop
    mov cx, 0x2DC6 ; CX = 0x2DC6 for roughly 1 second delay
    call delay_1s
    call delay_1s
    call delay_1s

    ; Load kernel
    call load_kernel

    ; Jump to kernel
    jmp 0x1000         ; Assuming kernel entry point is at 0x1000

delay_1s:
    push cx
    mov cx, 0xFFFF    ; Inner loop counter
delay_loop:
    loop delay_loop   ; Decrement CX and loop if not zero
    pop cx
    loop delay_1s     ; Decrement outer loop counter and loop if not zero
    ret

load_kernel:
    ; Load kernel from disk to 0x1000:0000
    mov bx, 0x1000     ; Load to address 0x1000:0000
    mov dh, 0          ; Track (head) number
    mov dl, 0x80       ; Drive number (first hard drive)
    mov ch, 0          ; Cylinder number
    mov cl, 2          ; Start reading from sector 2

    ; Read 18 sectors (9KB kernel)
    mov ah, 0x02       ; BIOS read sectors
    mov al, 18         ; Number of sectors to read
    mov es, bx         ; ES:BX -> destination segment:offset
    mov bx, 0x0000     ; Offset in segment

    int 0x13           ; BIOS disk interrupt

    jc disk_error      ; Jump if carry flag is set (error)
    ret

disk_error:
    mov si, error_msg
    call print_string
    jmp $

print_string:
    mov ah, 0x0E       ; BIOS teletype service
.next_char:
    lodsb              ; Load next character from SI into AL
    cmp al, 0
    je .done           ; If character is null, end of string
    int 0x10           ; Print character
    jmp .next_char
.done:
    ret

message db 'AnimikhaOS is booting', 0  ; Null-terminated string
error_msg db 'Disk read error!', 0     ; Null-terminated error message

; Bootloader signature (magic number for bootable media)
times 510 - ($ - $$) db 0     ; Fill up to 510 bytes
dw 0xAA55                     ; Bootloader signature